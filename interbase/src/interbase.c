/*
 *       File:    mem.c
 *       Purpose: dbi extension to Tcl: interbase backend
 *       Author:  Copyright (c) 1998 Peter De Rijk
 *
 *       See the file "README" for information on usage and redistribution
 *       of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "interbase.h"

void Tcl_GetCommandFullName(
    Tcl_Interp *interp,
    Tcl_Command command,
    Tcl_Obj *objPtr);

typedef struct vary {
	short vary_length;
	char vary_string[1];
} VARY;

int Dbi_interbase_Clone(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	Tcl_Obj *name);

/******************************************************************/

int dbi_Interbase_autocommit_state(
	dbi_Interbase_Data *dbdata)
{
	return dbdata->autocommit;
}

int dbi_Interbase_autocommit(
	dbi_Interbase_Data *dbdata,
	int state)
{
	dbdata->autocommit = state;
	return state;
}

isc_tr_handle *dbi_Interbase_trans(
	dbi_Interbase_Data *dbdata)
{
	return &(dbdata->trans);
}

int dbi_Interbase_trans_state(
	dbi_Interbase_Data *dbdata)
{
	if (dbdata->trans == NULL) {return 0;} else {return 1;}
}

/*
 * Hopefully I will be able to use the following later, once I find out a way to let 2 stmts
 * share a transaction without messing up eachothers resultset
 */

# ifdef never
int dbi_Interbase_autocommit_state(
	dbi_Interbase_Data *dbdata)
{
	if (dbdata->parent != NULL) {
		return dbdata->parent->autocommit;
	} else {
		return dbdata->autocommit;
	}
}

int dbi_Interbase_autocommit(
	dbi_Interbase_Data *dbdata,
	int state)
{
	if (dbdata->parent != NULL) {
		dbdata->parent->autocommit = state;
	} else {
		dbdata->autocommit = state;
	}
	return state;
}

isc_tr_handle *dbi_Interbase_trans(
	dbi_Interbase_Data *dbdata)
{
		return &(dbdata->trans);
	if (dbdata->parent != NULL) {
		return &(dbdata->parent->trans);
	} else {
		return &(dbdata->trans);
	}
}

int dbi_Interbase_trans_state(
	dbi_Interbase_Data *dbdata)
{
		if (dbdata->trans == NULL) {return 0;} else {return 1;}
	if (dbdata->parent != NULL) {
		if (dbdata->parent->trans == NULL) {return 0;} else {return 1;}
	} else {
		if (dbdata->trans == NULL) {return 0;} else {return 1;}
	}
}
#endif

int dbi_Interbase_String_Tolower(
	Tcl_Interp *interp,
	Tcl_Obj **stringObj)
{
	char *temp;
	char *string;
	int i,len;
	string = Tcl_GetStringFromObj(*stringObj,&len);
	temp = Tcl_Alloc(len*sizeof(char));
	for (i = 0; i < len; i++) {
		if ((string[i] > 64) && (string[i] < 91)) {
			temp[i] = string[i] + 32;
		} else {
			temp[i] = string[i];
		}
	}
	*stringObj = Tcl_NewStringObj(temp,len);
	Tcl_Free(temp);
	return TCL_OK;
}

int dbi_Interbase_Reconnect(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata)
{
	char *string=NULL;
	int len,error;
	if (dbdata->db == NULL) {return TCL_ERROR;}
	error = isc_detach_database(dbdata->status, &(dbdata->db));
	dbdata->stmt = NULL;
	dbdata->trans = NULL;
	dbdata->cursor_open = 0;
	string = Tcl_GetStringFromObj(dbdata->database,&len);
	error = isc_attach_database(dbdata->status, len, string, &(dbdata->db), dbdata->dpbpos, dbdata->dpb);
	if (error) {
		Tcl_AppendResult(interp,"reconnection to database \"",
			Tcl_GetStringFromObj(dbdata->database,NULL) , "\" failed\n", NULL);
		goto error;
	}
	error = isc_dsql_allocate_statement(dbdata->status, &(dbdata->db), &(dbdata->stmt));
	if (error) {
		Tcl_AppendResult(interp,"Interbase error on statement allocation:\n", NULL);
		goto error;
	}
	if (dbdata->clonesnum) {
		dbi_Interbase_Data *clone_dbdata;
		int i;
		for (i = 0 ; i < dbdata->clonesnum; i++) {
			clone_dbdata = dbdata->clones[i];
			clone_dbdata->parent = dbdata;
			clone_dbdata->dpblen = dbdata->dpblen;
			clone_dbdata->dpb = dbdata->dpb;
			clone_dbdata->database = dbdata->database;
			clone_dbdata->dpbpos = dbdata->dpbpos;
			clone_dbdata->db = dbdata->db;
			error = isc_dsql_allocate_statement(clone_dbdata->status, &(clone_dbdata->db), &(clone_dbdata->stmt));
			if (error) {
				Tcl_AppendResult(interp,"Interbase error on statement allocation:\n", NULL);
				goto error;
			}
		}
	}
	return TCL_OK;
	error:
		return TCL_ERROR;
}

int dbi_Interbase_Free_out_sqlda(
	dbi_Interbase_Data *dbdata)
{
	XSQLDA *sqlda = dbdata->out_sqlda;
	int i;
	if (dbdata->out_sqlda_filled) {
		for(i = 0 ; i < sqlda->sqld ; i++) {
			if (sqlda->sqlvar[i].sqldata != NULL) Tcl_Free((char *)sqlda->sqlvar[i].sqldata);
			if (sqlda->sqlvar[i].sqltype & 1) Tcl_Free((char *)sqlda->sqlvar[i].sqlind);
		}
		sqlda->sqld = 0;
	}
	dbdata->out_sqlda_filled = 0;
	return TCL_OK;
}

int dbi_Interbase_Prepare_out_sqlda(
	Tcl_Interp *interp,
	XSQLDA *out_sqlda)
{
	XSQLVAR *var;
	int i,dtype;
	for (i = 0, var = out_sqlda->sqlvar; i < out_sqlda->sqld; i++, var++) {
		dtype = (var->sqltype & ~1); /* drop flag bit for now */
		switch(dtype) {
			case SQL_TEXT:
				var->sqldata = (char *)Tcl_Alloc(sizeof(char)*var->sqllen+1);
				break;
			case SQL_VARYING:
				var->sqldata = (char *)Tcl_Alloc(sizeof(short) + sizeof(char)*var->sqllen + 1);
				break;
			case SQL_SHORT:
				var->sqldata = (char*)Tcl_Alloc(sizeof(short));
				break;
			case SQL_LONG:
				var->sqldata = (char *)Tcl_Alloc(sizeof(long));
				break;
			case SQL_FLOAT:
				var->sqldata = (char*)Tcl_Alloc(sizeof(float));
				break;
			case SQL_DOUBLE:
				var->sqldata = (char*)Tcl_Alloc(sizeof(double));
				break;
			case SQL_D_FLOAT:
				var->sqldata = (char*)Tcl_Alloc(sizeof(double));
				break;
			case SQL_TIMESTAMP:
				var->sqldata = (char*)Tcl_Alloc(sizeof(ISC_TIMESTAMP));
				break;
			case SQL_BLOB:
			case SQL_ARRAY:
			case SQL_QUAD:
				var->sqldata = (char*)Tcl_Alloc(sizeof(ISC_QUAD));
				break;
			case SQL_TYPE_TIME:
				var->sqldata = (char*)Tcl_Alloc(sizeof(ISC_TIME));
				break;
			case SQL_TYPE_DATE:
				var->sqldata = (char*)Tcl_Alloc(sizeof(ISC_DATE));
				break;
			case SQL_INT64:
				var->sqldata = (char*)Tcl_Alloc(sizeof(ISC_INT64));
				break;
			default:
				{
				int j;
				for (j = 0, var = out_sqlda->sqlvar; j < i; j++, var++) {
					if (var->sqldata != NULL) Tcl_Free((char *)var->sqldata);
					if (var->sqltype & 1) Tcl_Free((char *)var->sqlind);
				}
				Tcl_AppendResult(interp,"unsupported return type", NULL);
				}
				return TCL_ERROR;
		}
		if (var->sqltype & 1) { /* allocate variable to hold NULL status */ 
			var->sqlind = (short *)Tcl_Alloc(sizeof(short));
		}
	}
	return TCL_OK;
}

int dbi_Interbase_Error(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata)
{
	ISC_STATUS *status_vector = dbdata->status;
	isc_stmt_handle *stmt = &(dbdata->stmt);
	char msg[512];
	long SQLCODE;
	int error;
	SQLCODE = isc_sqlcode(status_vector);
	isc_sql_interprete(SQLCODE, msg, 512);
	while(isc_interprete(msg + 1, &status_vector)) {
		Tcl_AppendResult(interp,msg+1," - ", NULL);
	}
	if (dbdata->cursor_open != 0) {
		error = isc_dsql_free_statement(status_vector, stmt, DSQL_close);
		dbdata->cursor_open = 0;
		dbi_Interbase_Free_out_sqlda(dbdata);
	}
	if (SQLCODE == -902) {
		error = dbi_Interbase_Reconnect(interp,dbdata);
		if (error) {
			Tcl_AppendResult(interp,"error while reconnecting\n", NULL);
			return TCL_ERROR;
		} else {
			Tcl_AppendResult(interp,"reconnected\n", NULL);
			return TCL_OK;
		}
	}
	if ((dbi_Interbase_autocommit_state(dbdata))&&(dbi_Interbase_trans_state(dbdata))) {
		error = isc_rollback_transaction(status_vector, dbi_Interbase_trans(dbdata));
		if (error) {
			Tcl_AppendResult(interp,"error rolling back transaction:\n", NULL);
			dbi_Interbase_Error(interp,dbdata);
			return TCL_ERROR;
		}
	}
	return SQLCODE;
}

int dbi_Interbase_Blob_Create(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	Tcl_Obj *data,
	ISC_QUAD *blob_id)
{
	ISC_STATUS *status_vector = dbdata->status;
	isc_blob_handle blob_handle = NULL;
	char *string;
	int error,pos,seglen,stringlen;
	string = Tcl_GetStringFromObj(data,&stringlen);
	seglen = stringlen;
	if (seglen > 8192) {seglen = 8192;}
	error = isc_create_blob2(status_vector, &(dbdata->db), dbi_Interbase_trans(dbdata), &blob_handle, blob_id, 0, NULL);
	if (error) {goto error;}
	pos = 0;
	while (pos < stringlen) {
		if ((pos+seglen) > stringlen) {seglen = stringlen - pos;}
		error = isc_put_segment(status_vector, &blob_handle, seglen, string+pos);
		if (error) {goto error;}
		pos += seglen;
	}
	error = isc_close_blob(status_vector, &blob_handle);
	if (error) {goto error;}
	return TCL_OK;
	error:
		dbi_Interbase_Error(interp,dbdata);
		return TCL_ERROR;
}

int dbi_Interbase_Blob_Get(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	ISC_QUAD *blob_id,
	int start,
	int end,
	Tcl_Obj **resultPtr)
{
	ISC_STATUS *status_vector = dbdata->status;
	ISC_STATUS blob_stat = 0;
	isc_blob_handle blob_handle = NULL;
	Tcl_Obj *result = NULL;
	char blob_segment[8192];
	int blob_segsize,error;
	unsigned short actual_seg_len;
	blob_segsize = 400;
	error = isc_open_blob2(status_vector, &(dbdata->db),  dbi_Interbase_trans(dbdata), &blob_handle, blob_id, 0, NULL);
	if (error) {dbi_Interbase_Error(interp,dbdata);goto error;}
	result = Tcl_NewStringObj("",0);
	actual_seg_len = 8192;
	while (actual_seg_len) {
		blob_stat = isc_get_segment(status_vector, &blob_handle,
			&actual_seg_len,8192,blob_segment);
		Tcl_AppendToObj(result,blob_segment,actual_seg_len);
		if (blob_stat == isc_segstr_eof) break;
		if (blob_stat) {dbi_Interbase_Error(interp,dbdata);goto error;}
	}
	if (isc_close_blob(status_vector, &blob_handle)) {
		dbi_Interbase_Error(interp,dbdata);
		goto error;
	}
	*resultPtr = result;
	return TCL_OK;
	error:
		if (result != NULL) {Tcl_DecrRefCount(result);}
		return TCL_ERROR;
}

int dbi_Interbase_Open(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	static char *switches[] = {"-user","-password","-role", (char *) NULL};
	char *string=NULL;
	int len,i,dpbpos,item=0,index,error;
	if (dbdata->db != NULL) {
		Tcl_AppendResult(interp,"dbi object has open connection, close first", NULL);
		goto error;
	}
	if (objc < 3) {
		Tcl_WrongNumArgs(interp,2,objv,"datasource ?options?");
		return TCL_ERROR;
	}
	dpbpos = 0;
	if (dbdata->dpb != NULL) {Tcl_Free(dbdata->dpb);}
	dbdata->dpblen = 200;
	dbdata->dpb = Tcl_Alloc(dbdata->dpblen*sizeof(char));
	dbdata->dpb[dpbpos++] = isc_dpb_version1;
	for (i = 3; i < objc; i++) {
		if (Tcl_GetIndexFromObj(interp, objv[i], switches, "option", 0, &index)!= TCL_OK) {
			return TCL_ERROR;
		}
		switch (index) {
			case 0:			/* -user */
				if (i == (objc)) {
					Tcl_AppendResult(interp,"\"-user\" option must be followed by the username",NULL);
					return TCL_ERROR;
				}
				string = Tcl_GetStringFromObj(objv[i+1], &len);
				item = isc_dpb_user_name;
				i++;
				break;
			case 1:			/* -password */
				if (i == (objc-1)) {
					Tcl_AppendResult(interp,"\"-password\" option must be followed by the password of the user",NULL);
					return TCL_ERROR;
				}
				string = Tcl_GetStringFromObj(objv[i+1], &len);
				item = isc_dpb_password;
				i++;
				break;
			case 2:			/* -role */
				if (i == (objc-1)) {
					Tcl_AppendResult(interp,"\"-role\" option must be followed by the role",NULL);
					return TCL_ERROR;
				}
				string = Tcl_GetStringFromObj(objv[i+1], &len);
				item = isc_dpb_sql_role_name;
				i++;
				break;
		}
		if (item != -1) {
			if ((dpbpos+len+2) > dbdata->dpblen) {
				dbdata->dpblen = dpbpos+len+2;
				dbdata->dpb = Tcl_Realloc(dbdata->dpb,dbdata->dpblen*sizeof(char));
			}
			dbdata->dpb[dpbpos++]=item;
			dbdata->dpb[dpbpos++]=len;
			strncpy(dbdata->dpb+dpbpos,string,len);
			dpbpos += len;
		}
	}
	dbdata->database = objv[2];
	dbdata->dpbpos = dpbpos;
	Tcl_IncrRefCount(dbdata->database);
	string = Tcl_GetStringFromObj(dbdata->database,&len);
	error = isc_attach_database(dbdata->status, len, string, &(dbdata->db), dpbpos, dbdata->dpb);
	if (error) {
		Tcl_AppendResult(interp,"connection to database \"",
			Tcl_GetStringFromObj(dbdata->database,NULL) , "\" failed:\n", NULL);
		Tcl_DecrRefCount(dbdata->database);dbdata->database=NULL;
		dbi_Interbase_Error(interp,dbdata);
		goto error;
	}
	error = isc_dsql_allocate_statement(dbdata->status, &(dbdata->db), &(dbdata->stmt));
	if (error) {
		Tcl_AppendResult(interp,"Interbase error on statement allocation:\n", NULL);
		dbi_Interbase_Error(interp,dbdata);
		goto error;
	}
	return TCL_OK;
	error:
		return TCL_ERROR;
}

int dbi_Interbase_Fetch_One(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	int pos,
	Tcl_Obj **result)
{
	XSQLDA *out_sqlda = dbdata->out_sqlda;
	Tcl_Obj *element = NULL;
	XSQLVAR *var;
	short dtype;
	int textend,error;
	var = out_sqlda->sqlvar+pos;
	if ((var->sqltype & 1) && (*var->sqlind < 0)) { /* process the NULL value here */
		element = NULL;
	} else { /* process the data instead */ 
		dtype = var->sqltype & ~1;
		switch (dtype) {
			case SQL_VARYING:
				{
				VARY *vary;
				vary = (VARY*) var->sqldata;
				textend = vary->vary_length;
				element = Tcl_NewStringObj(vary->vary_string,vary->vary_length);
				break;
				}
			case SQL_TEXT:
				textend = var->sqllen-1;
				while (var->sqldata[textend] == ' ') {if (textend <= 0) break;textend--;}
				element = Tcl_NewStringObj(var->sqldata,textend+1);
				break;
			case SQL_SHORT:
			case SQL_LONG:
			case SQL_INT64:
				{
				ISC_INT64 value = 0;
				short dscale;
				switch (dtype) {
					case SQL_SHORT:
						value = (ISC_INT64) *(short *) var->sqldata;
						break;
					case SQL_LONG:
						value = (ISC_INT64) *(long *) var->sqldata;
						break;
					case SQL_INT64:
						value = (ISC_INT64) *(ISC_INT64 *) var->sqldata;
						break;
				}
				dscale = var->sqlscale;
				if (dscale < 0) {
					ISC_INT64 tens;
					short i;
					tens = 1;
					for (i = 0; i > dscale; i--) tens *= 10;
					element = Tcl_NewDoubleObj(value/tens);
				} else if (dscale > 1) {
					element = Tcl_NewIntObj(value*dscale);
				} else {
					element = Tcl_NewIntObj(value);
				}
				}
				break;
			case SQL_FLOAT:
				element = Tcl_NewDoubleObj((double)*(float *)var->sqldata);
				break;
			case SQL_D_FLOAT:
			case SQL_DOUBLE:
				element = Tcl_NewDoubleObj(*(double *)var->sqldata);
				break;
			case SQL_TIMESTAMP:
				{
				struct tm times;
				char date_s[24];
				isc_decode_timestamp((ISC_TIMESTAMP ISC_FAR *) var->sqldata,&times);
				sprintf(date_s, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
						times.tm_year + 1900,
						times.tm_mon+1,
						times.tm_mday,
						times.tm_hour,
						times.tm_min,
						times.tm_sec,
						(int)((ISC_TIMESTAMP *) var->sqldata)->timestamp_time%100000);
				element = Tcl_NewStringObj(date_s,23);
				}
				break;
			case SQL_BLOB:
				{
				ISC_QUAD bid;
                bid = *(ISC_QUAD *)var->sqldata;
/*fprintf(stdout, "%08lx:%08lx\n", bid.isc_quad_high, bid.isc_quad_low);fflush(stdout);*/
				error = dbi_Interbase_Blob_Get(interp,dbdata,&bid,0,-1,&element);
				if (error) {goto error;}
				}
				break;
			case SQL_ARRAY:
			case SQL_QUAD:
				{
				ISC_QUAD bid;
				char blob_s[18];
                bid = *(ISC_QUAD *)var->sqldata;
                sprintf(blob_s, "%08lx:%08lx", bid.isc_quad_high, bid.isc_quad_low);
				element = Tcl_NewStringObj(blob_s,17);
				}
				break;
			case SQL_TYPE_TIME:
				{
				struct tm times;
				char date_s[13];
				isc_decode_sql_time((ISC_TIME *)var->sqldata, &times);
				sprintf(date_s, "%02d:%02d:%02d.%03d",
						times.tm_hour,
						times.tm_min,
						times.tm_sec,
						(int)(*((ISC_TIME *)var->sqldata)) % 100000);
				element = Tcl_NewStringObj(date_s,12);
				}
				break;
			case SQL_TYPE_DATE:
				{
				struct tm times;
				char date_s[11];
				isc_decode_sql_date((ISC_DATE *)var->sqldata, &times);
				sprintf(date_s, "%04d-%02d-%02d",
						times.tm_year + 1900,
						times.tm_mon+1,
						times.tm_mday);
				element = Tcl_NewStringObj(date_s,10);
				}
				break;
			default:
				Tcl_AppendResult(interp,"unsupported type", NULL);
				goto error;
		}
	}
	*result = element;
	return TCL_OK;
	error:
		if (element != NULL) Tcl_DecrRefCount(element);
		return TCL_ERROR;
}

int dbi_Interbase_Fetch_Row(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	XSQLDA *out_sqlda,
	Tcl_Obj *nullvalue,
	Tcl_Obj **result)
{
	Tcl_Obj *line = NULL, *element = NULL;
	int i,error;
	line = Tcl_NewListObj(0,NULL);
	for (i = 0; i < out_sqlda->sqld; i++) {
		error = dbi_Interbase_Fetch_One(interp,dbdata,i,&element);
		if (error) {goto error;}
		if (element == NULL) {element = nullvalue;}
		error = Tcl_ListObjAppendElement(interp,line, element);
		if (error) goto error;
		element = NULL;
	}
	*result = line;
	return TCL_OK;
	error:
		if (line != NULL) Tcl_DecrRefCount(line);
		if (element != NULL) Tcl_DecrRefCount(element);
		return TCL_ERROR;
}

int dbi_Interbase_Fetch_Fields(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	XSQLDA *out_sqlda,
	Tcl_Obj **result)
{
	Tcl_Obj *line = NULL, *element = NULL;
	XSQLVAR *var;
	int i,error;
	line = Tcl_NewListObj(0,NULL);
	for (i = 0; i < out_sqlda->sqld; i++) {
		var = out_sqlda->sqlvar+i;
		element = Tcl_NewStringObj(var->sqlname,var->sqlname_length);
		error = Tcl_ListObjAppendElement(interp,line, element);
		if (error) goto error;
		element = NULL;
	}
	*result = line;
	return TCL_OK;
	error:
		if (line != NULL) Tcl_DecrRefCount(line);
		if (element != NULL) Tcl_DecrRefCount(element);
		return TCL_ERROR;
}

int dbi_Interbase_ToResult(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	XSQLDA *out_sqlda,
	Tcl_Obj *nullvalue)
{
	ISC_STATUS *status_vector = dbdata->status;
	isc_stmt_handle *stmt = &(dbdata->stmt);
	Tcl_Obj *result = NULL, *line = NULL;
    long fetch_stat;
	int error;
	result = Tcl_NewListObj(0,NULL);
	while (1) {
		fetch_stat = isc_dsql_fetch(status_vector, stmt, SQL_DIALECT_V6, out_sqlda);
		if (fetch_stat) break;
		error = dbi_Interbase_Fetch_Row(interp,dbdata,out_sqlda,nullvalue,&line);
		if (error) {goto error;}
		error = Tcl_ListObjAppendElement(interp,result, line);
		if (error) goto error;
		line = NULL;
	}
	Tcl_SetObjResult(interp, result);
	return TCL_OK;
	error:
		if (result != NULL) Tcl_DecrRefCount(result);
		if (line != NULL) Tcl_DecrRefCount(line);
		return TCL_ERROR;
}

int dbi_Interbase_Transaction_Commit(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata)
{
	ISC_STATUS *status_vector = dbdata->status;
	isc_stmt_handle *stmt = &(dbdata->stmt);
	int error;
	if (dbdata->cursor_open != 0) {
		error = isc_dsql_free_statement(status_vector, stmt, DSQL_close);
		dbdata->cursor_open = 0;
		dbi_Interbase_Free_out_sqlda(dbdata);
	}
	if ((dbi_Interbase_autocommit_state(dbdata))&&(dbi_Interbase_trans_state(dbdata))) {
		error = isc_commit_transaction(dbdata->status, dbi_Interbase_trans(dbdata));
		if (error) {
			Tcl_AppendResult(interp,"error committing transaction:\n", NULL);
			dbi_Interbase_Error(interp,dbdata);
			return TCL_ERROR;
		}
	}
	return TCL_OK;
}

int dbi_Interbase_Transaction_Rollback(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata)
{
	ISC_STATUS *status_vector = dbdata->status;
	isc_stmt_handle *stmt = &(dbdata->stmt);
	int error;
	if (dbdata->cursor_open != 0) {
		error = isc_dsql_free_statement(status_vector, stmt, DSQL_close);
		dbdata->cursor_open = 0;
/*
		if (error) {
			Tcl_AppendResult(interp,"Interbase error:\n", NULL);
			dbi_Interbase_Error(interp,dbdata);
			return TCL_ERROR;
		}
*/
		dbi_Interbase_Free_out_sqlda(dbdata);
	}
	if ((dbi_Interbase_autocommit_state(dbdata))&&(dbi_Interbase_trans_state(dbdata))) {
		error = isc_rollback_transaction(dbdata->status, dbi_Interbase_trans(dbdata));
		if (error) {
			Tcl_AppendResult(interp,"error rolling back transaction:\n", NULL);
			dbi_Interbase_Error(interp,dbdata);
			return TCL_ERROR;
		}
	}
	return TCL_OK;
}

int dbi_Interbase_Transaction_Start(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata)
{
	int error;
	error = dbi_Interbase_Transaction_Commit(interp,dbdata);
	if (error) {
		dbi_Interbase_Error(interp,dbdata);
		return TCL_ERROR;
	}
	if (dbi_Interbase_autocommit_state(dbdata)) {
		static char isc_tpb[] = {
			isc_tpb_version3,
			isc_tpb_read_committed,
			isc_tpb_rec_version
		};
		error = isc_start_transaction(dbdata->status, dbi_Interbase_trans(dbdata),1,&(dbdata->db),(unsigned short) sizeof(isc_tpb),isc_tpb);
		if (error) {
			long SQLCODE;
			SQLCODE = isc_sqlcode(dbdata->status);
			if (SQLCODE == -902) {
				error = dbi_Interbase_Reconnect(interp,dbdata);
				if (error) {return TCL_ERROR;}
				error = isc_start_transaction(dbdata->status, dbi_Interbase_trans(dbdata),1,&(dbdata->db), (unsigned short) sizeof(isc_tpb),isc_tpb);
				if (!error) {return TCL_OK;}
			}
			Tcl_AppendResult(interp,"error starting transaction:\n", NULL);
			dbi_Interbase_Error(interp,dbdata);
			return TCL_ERROR;
		}
	}
	return TCL_OK;
}

int dbi_Interbase_Process_statement(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	int cmdlen,
	char *cmdstring,
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	ISC_STATUS *status_vector = dbdata->status;
	isc_stmt_handle *stmt = &(dbdata->stmt);
	XSQLVAR *var = NULL;
	static char stmt_info[] = { isc_info_sql_stmt_type };
	char info_buffer[20], *nullstring, *string;
	short l;
	int error,n = 0,nulllen,len,dtype,size,temp,ms,i,ipos = 0;
	dbi_Interbase_Free_out_sqlda(dbdata);
	error = isc_dsql_prepare(status_vector, dbi_Interbase_trans(dbdata), stmt, cmdlen, cmdstring, SQL_DIALECT_V6, dbdata->out_sqlda);
	if (error) {
		dbi_Interbase_Error(interp,dbdata);
		goto error;
	}
	/* Get information about input of cmd */
	error = isc_dsql_describe_bind(status_vector, stmt, SQL_DIALECT_V6, dbdata->in_sqlda);
/* fprintf(stdout,"objc=%d, sqld=%d\n",objc,dbdata->in_sqlda->sqld);fflush(stdout); */
	if (objc != dbdata->in_sqlda->sqld) {
		Tcl_AppendResult(interp,"wrong number of arguments given to exec",NULL);
		goto error;
	}
	if (dbdata->in_sqlda->sqld > dbdata->in_sqlda->sqln) {
		n = dbdata->in_sqlda->sqld;
		Tcl_Free((char *)dbdata->in_sqlda);
		dbdata->in_sqlda = (XSQLDA *)Tcl_Alloc(XSQLDA_LENGTH(n));
		dbdata->in_sqlda->sqln = n;
		dbdata->in_sqlda->version = SQLDA_VERSION1;
		isc_dsql_describe_bind(status_vector, stmt, SQL_DIALECT_V6, dbdata->in_sqlda);
	}
	nullstring = Tcl_GetStringFromObj(nullvalue,&nulllen);
	for (ipos = 0, var = dbdata->in_sqlda->sqlvar; ipos < dbdata->in_sqlda->sqld; ipos++, var++) {
		if (var->sqltype & 1) {
			var->sqlind = (short *)Tcl_Alloc(sizeof(short));
			*(var->sqlind) = 1;
			string = Tcl_GetStringFromObj(objv[ipos],&len);
			if ((len == nulllen)&&(strncmp(string,nullstring,len) == 0)) {
				*(var->sqlind) = -1;
			} else {
				*(var->sqlind) = 0;
			}
		}
		dtype = (var->sqltype & ~1);
/* fprintf(stdout,"ipos=%d, dtype=%d\n",ipos,dtype);fflush(stdout);*/
		switch(dtype) {
			case SQL_VARYING:
				var->sqltype = SQL_TEXT;
			case SQL_TEXT:
				var->sqldata = Tcl_GetStringFromObj(objv[ipos],&size);
				if (size > 32767) {
					Tcl_AppendResult(interp,"String argument to large");
					goto error;
				}
				var->sqllen = (short)size;
				break;
			case SQL_SHORT:
				var->sqldata = (char *)Tcl_Alloc(sizeof(short));
				error = Tcl_GetIntFromObj(interp,objv[ipos],&temp);
				if (error) {goto error;}
				if (var->sqlscale < 0) {
					ISC_INT64 tens;
					short i;
					tens = 1;
					for (i = 0; i > var->sqlscale; i--) tens *= 10;
					*(short *)var->sqldata = (short)(temp*tens);
				} else if (var->sqlscale > 1) {
					*(short *)var->sqldata = (short)(temp/var->sqlscale);
				} else {
					*(short *)var->sqldata = (short)temp;
				}
				break;
			case SQL_LONG:
				var->sqldata = (char *)Tcl_Alloc(sizeof(long));
				error = Tcl_GetIntFromObj(interp,objv[ipos],&temp);
				*(long *)(var->sqldata) = (long)temp;
				if (error) {goto error;}
				if (var->sqlscale < 0) {
					ISC_INT64 tens;
					short i;
					tens = 1;
					for (i = 0; i > var->sqlscale; i--) tens *= 10;
					*(long *)var->sqldata = (long)(temp*tens);
				} else if (var->sqlscale > 1) {
					*(long *)var->sqldata = (long)(temp/var->sqlscale);
				} else {
					*(long *)var->sqldata = (long)temp;
				}
				break;
			case SQL_INT64:
				break;
				{
				double temp;
				var->sqldata = (char *)Tcl_Alloc(sizeof(long));
				error = Tcl_GetDoubleFromObj(interp,objv[ipos],&temp);
				if (error) {goto error;}
				if (var->sqlscale < 0) {
					ISC_INT64 tens;
					short i;
					tens = 1;
					for (i = 0; i > var->sqlscale; i--) tens *= 10;
					*(ISC_INT64 *)var->sqldata = (ISC_INT64)temp*(ISC_INT64)tens;
				} else if (var->sqlscale > 1) {
					*(ISC_INT64 *)var->sqldata = (ISC_INT64)temp/(ISC_INT64)var->sqlscale;
				} else {
					*(ISC_INT64 *)var->sqldata = (ISC_INT64)temp;
				}
				}
				break;
			case SQL_FLOAT:
				{
				double temp;
				var->sqldata = (char *)Tcl_Alloc(sizeof(long));
				error = Tcl_GetDoubleFromObj(interp,objv[ipos],&temp);
				if (error) {goto error;}
				*(float *)(var->sqldata) = (float)temp;
				}
				break;
			case SQL_D_FLOAT:
			case SQL_DOUBLE:
				var->sqldata = (char *)Tcl_Alloc(sizeof(long));
				error = Tcl_GetDoubleFromObj(interp,objv[ipos],(double *)(var->sqldata));
				if (error) {goto error;}
				break;
			case SQL_TIMESTAMP:
				{
				struct tm times;
				int ms;
				var->sqldata = (char *)Tcl_Alloc(sizeof(ISC_TIMESTAMP));
				string = Tcl_GetStringFromObj(objv[ipos],&len);
				times.tm_year = 0;
				times.tm_mon = 0;
				times.tm_mday = 0;
				times.tm_hour = 0;
				times.tm_min = 0;
				times.tm_sec = 0;
				sscanf(string,"%d-%d-%d %d:%d:%d.%d",&times.tm_year,&times.tm_mon,&times.tm_mday,&times.tm_hour,&times.tm_min,&times.tm_sec,&ms);
				times.tm_year -= 1900;
				times.tm_mon -= 1;
				isc_encode_timestamp(&times,(ISC_TIMESTAMP *) var->sqldata);
				}
				break;
			case SQL_BLOB:
				{
				ISC_QUAD *blob_id;
				blob_id = (ISC_QUAD *)Tcl_Alloc(sizeof(ISC_QUAD));
				error = dbi_Interbase_Blob_Create(interp,dbdata,objv[ipos],blob_id);
				var->sqldata = (char *)blob_id;
/*                fprintf(stdout, "%08lx:%08lx\n", blob_id->isc_quad_high, blob_id->isc_quad_low);fflush(stdout);*/
				if (error) {dbi_Interbase_Error(interp,dbdata);goto error;}
				}
				break;
			case SQL_ARRAY:
			case SQL_QUAD:
				/*
				 * Print the blob id on blobs or arrays 
				 */
				{
				var->sqldata = (char *)Tcl_Alloc(sizeof(ISC_QUAD));
				}
				break;
			case SQL_TYPE_TIME:
				{
				struct tm times;
				var->sqldata = (char *)Tcl_Alloc(sizeof(ISC_TIME));
				string = Tcl_GetStringFromObj(objv[ipos],&len);
				times.tm_year = 0;
				times.tm_mon = 0;
				times.tm_mday = 0;
				times.tm_hour = 0;
				times.tm_min = 0;
				times.tm_sec = 0;
				sscanf(string,"%d:%d:%d.%d",&times.tm_hour,&times.tm_min,&times.tm_sec,&ms);
				times.tm_year -= 1900;
				isc_encode_sql_time(&times,(ISC_TIME *) var->sqldata);
				}
				break;
			case SQL_TYPE_DATE:
				{
				struct tm times;
				var->sqldata = (char *)Tcl_Alloc(sizeof(ISC_DATE));
				string = Tcl_GetStringFromObj(objv[ipos],&len);
				times.tm_year = 0;
				times.tm_mon = 0;
				times.tm_mday = 0;
				times.tm_hour = 0;
				times.tm_min = 0;
				times.tm_sec = 0;
				sscanf(string,"%d-%d-%d",&times.tm_year,&times.tm_mon,&times.tm_mday);
				times.tm_year -= 1900;
				times.tm_mon -= 1;
				isc_encode_sql_date(&times,(ISC_DATE *) var->sqldata);
				}
				break;
			default:
				Tcl_AppendResult(interp,"unsupported type", NULL);
				goto error;
		}
	}
	/* Get information about output of cmd */
	if (dbdata->out_sqlda->sqld == 0) {
		/* What is the statement type of this statement? 
		**
		** stmt_info is a 1 byte info request.  info_buffer is a buffer
		** large enough to hold the returned info packet
		** The info_buffer returned contains a isc_info_sql_stmt_type in the first byte, 
		** two bytes of length, and a statement_type token.
		*/
		long statement_type;
		error = isc_dsql_sql_info(status_vector, stmt, sizeof(stmt_info), stmt_info, sizeof(info_buffer), info_buffer);
		if (!error)	{
			l = (short) isc_vax_integer((char ISC_FAR *)info_buffer + 1, 2);
			statement_type = isc_vax_integer((char ISC_FAR *)info_buffer + 3, l);
		} else {
			statement_type = 0;
		}
		switch (statement_type) {
			case isc_info_sql_stmt_start_trans:
				error = dbi_Interbase_Transaction_Commit(interp,dbdata);
				if (error) {goto error;}
				error = isc_dsql_execute_immediate(status_vector, &(dbdata->db), 
					dbi_Interbase_trans(dbdata), 0, cmdstring, SQL_DIALECT_V6, dbdata->in_sqlda);
				if (error) {dbi_Interbase_Error(interp,dbdata);goto error;}
				dbi_Interbase_autocommit(dbdata,0);
				goto clean;
				break;
			case isc_info_sql_stmt_commit:
				error = isc_dsql_execute_immediate(status_vector, &(dbdata->db), 
					dbi_Interbase_trans(dbdata), 0, cmdstring, SQL_DIALECT_V6, dbdata->in_sqlda);
				dbi_Interbase_autocommit(dbdata,1);
				if (error) {dbi_Interbase_Error(interp,dbdata);goto error;}
				goto clean;
				break;
			case isc_info_sql_stmt_rollback:
				error = isc_dsql_execute_immediate(status_vector, &(dbdata->db), 
					dbi_Interbase_trans(dbdata), 0, cmdstring, SQL_DIALECT_V6, dbdata->in_sqlda);
				dbi_Interbase_autocommit(dbdata,1);
				if (error) {dbi_Interbase_Error(interp,dbdata);goto error;}
				goto clean;
				break;
		}
	} else {
		/* expand out_sqlda as necessary */
		if (dbdata->out_sqlda->sqld > dbdata->out_sqlda->sqln) {
			n = dbdata->out_sqlda->sqld;
			Tcl_Free((char *)dbdata->out_sqlda);
			dbdata->out_sqlda = (XSQLDA *)Tcl_Alloc(XSQLDA_LENGTH(n));
			if (dbdata->out_sqlda == NULL) {
				Tcl_AppendResult(interp,"memory allocation error", NULL);
				goto error;
			}
			dbdata->out_sqlda->sqln = n;
			dbdata->out_sqlda->sqld = 0;
			dbdata->out_sqlda->version = SQLDA_VERSION1;
			error = isc_dsql_describe(status_vector, stmt, SQL_DIALECT_V6, dbdata->out_sqlda);
			if (error) {
				Tcl_AppendResult(interp,"database error in describe\n", NULL);
				goto error;
			}
		}
		error = dbi_Interbase_Prepare_out_sqlda(interp,dbdata->out_sqlda);
		if (error) {
			Tcl_AppendResult(interp,"database error in prepare\n", NULL);
			goto error;
		}
		dbdata->out_sqlda_filled = 1;
		dbdata->cursor_open = 1;
	}
	/* Execute prepared stmt */
	error = isc_dsql_execute(status_vector, dbi_Interbase_trans(dbdata), stmt, SQL_DIALECT_V6, dbdata->in_sqlda);
	if (error) {
		Tcl_AppendResult(interp,"database error in execute: ", NULL);
		dbi_Interbase_Error(interp,dbdata);
		goto error;
	}
	clean:
	for(i = 0 ; i < ipos ; i++) {
		dtype = (dbdata->in_sqlda->sqlvar[i].sqltype & ~1);
		switch(dtype) {
			case SQL_TEXT:
			case SQL_VARYING:
				break;
			default:
				Tcl_Free((char *)dbdata->in_sqlda->sqlvar[i].sqldata);
		}
		if (dbdata->in_sqlda->sqlvar[i].sqltype & 1) Tcl_Free((char *)dbdata->in_sqlda->sqlvar[i].sqlind);
	}
	return TCL_OK;
	error:
		{
		Tcl_Obj *temp;
		for(i = 0 ; i < ipos ; i++) {
			dtype = (dbdata->in_sqlda->sqlvar[i].sqltype & ~1);
			switch(dtype) {
				case SQL_TEXT:
				case SQL_VARYING:
					break;
				default:
					Tcl_Free((char *)dbdata->in_sqlda->sqlvar[i].sqldata);
			}
			if (dbdata->in_sqlda->sqlvar[i].sqltype & 1) Tcl_Free((char *)dbdata->in_sqlda->sqlvar[i].sqlind);
		}
		temp = Tcl_NewStringObj(cmdstring,cmdlen);
		Tcl_AppendResult(interp," while executing command: \"",	Tcl_GetStringFromObj(temp,NULL), "\"", NULL);
		Tcl_DecrRefCount(temp);
		}
		return TCL_ERROR;		
}

int dbi_Interbase_Line_empty(
	char *find,char *end)
{
	find += 3;
	while (find < end) {
		find++;
		if ((*find != ' ')&&(*find != '\t')) {
			break;
		}
	}
	if (find == end) {
		return 1;
	} else {
		return 0;
	}
}

int dbi_Interbase_Find_Prev(
	char *string,int pos,char *find)
{
	int len;
	len = strlen(find)-1;
	while (pos >= 0) {
		pos--;
		if ((string[pos] != ' ')&&(string[pos] != '\t')) {
			break;
		}
	}
	if (len > pos) {return 0;}
	while (len >= 0) {
		if (string[pos] != find[len]) {return 0;}
		len--;
		pos--;
	}
	return 1;
}

int dbi_Interbase_Exec(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	Tcl_Obj *cmd,
	int usefetch,
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	ISC_STATUS *status_vector = dbdata->status;
	isc_stmt_handle *stmt = &(dbdata->stmt);
	char *cmdstring = NULL,*nextline;
	int error,cmdlen,len;
	if (dbdata->db == NULL) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		return TCL_ERROR;
	}
	cmdstring = Tcl_GetStringFromObj(cmd,&cmdlen);
	if (cmdlen == 0) {return TCL_OK;}
	if (nullvalue == NULL) {
		nullvalue = Tcl_NewObj();
	}
	Tcl_IncrRefCount(nullvalue);
	if (dbdata->out_sqlda->sqld != 0) {
		if (dbi_Interbase_Transaction_Commit(interp,dbdata) != TCL_OK) {goto error;}
	}
	if (dbi_Interbase_Transaction_Start(interp,dbdata) != TCL_OK) {goto error;}
	nextline = strchr(cmdstring,';');
	if (nextline == NULL) {
		error = dbi_Interbase_Process_statement(interp,dbdata,cmdlen,cmdstring,nullvalue,objc,objv);
		if (error) {dbi_Interbase_autocommit(dbdata,1);goto error;}
	} else {
		char *find;
		int i,blevel,level,start,prevline;
		if (objc != 0) {
			Tcl_AppendResult(interp,"multiple commands cannot take arguments\n", NULL);
			goto error;
		}
		i = 0;
		blevel = 0;
		level = 0;
		while (i <= cmdlen) {
			if ((cmdstring[i] != ' ')&&(cmdstring[i] != '\n')&&(cmdstring[i] != '\t')) break;
			i++;
		}
		start = i;
		prevline = i;
		while (1) {
			if (cmdstring[i] == '\n') {
				/* fprintf(stdout,"# %d,%d %*.*s\n",blevel,level,i-prevline,i-prevline,cmdstring+prevline);fflush(stdout); */
				if (dbi_Interbase_Find_Prev(cmdstring+prevline,i-prevline,"begin")) {
					blevel++;
				} else if (dbi_Interbase_Find_Prev(cmdstring+prevline,i-prevline,"BEGIN")) {
					blevel++;
				} else if (dbi_Interbase_Find_Prev(cmdstring+prevline,i-prevline,"end")) {
					blevel--;
				} else if (dbi_Interbase_Find_Prev(cmdstring+prevline,i-prevline,"END")) {
					blevel--;
				}
				prevline = i+1;
			} else if ((cmdstring[i] == ';')||(i == cmdlen)) {
				len = i - start;
				if (len == 0) break;
				while (prevline <= i) {
					if ((cmdstring[prevline] != ' ')&&(cmdstring[prevline] != '\n')&&(cmdstring[prevline] != '\t')) break;
					prevline++;
				}
				if ((len > 7) && ((strncmp(cmdstring+prevline,"declare",7) == 0) || (strncmp(cmdstring+prevline,"DECLARE",7) == 0))) {
					i++;continue;
				}
				if (dbi_Interbase_Find_Prev(cmdstring+prevline,i-prevline,"end")) {
					blevel--;
				} else if (dbi_Interbase_Find_Prev(cmdstring+prevline,i-prevline,"END")) {
					blevel--;
				}				find = strstr(cmdstring+prevline,"end");
				if (((level != 0)||(blevel != 0))&&(i != cmdlen)) {
					i++;
					continue;
				}
				/* fprintf(stdout,"** %*.*s\n",i-start,i-start,cmdstring+start);fflush(stdout); */
				error = dbi_Interbase_Process_statement(interp,dbdata,i-start,cmdstring+start,nullvalue,0,NULL);
				if (error) {dbi_Interbase_autocommit(dbdata,1); goto error;}
				if (i == cmdlen) break;
				while (i < cmdlen) {
					i++;
					if (i == cmdlen) break;
					if ((cmdstring[i] != ' ')&&(cmdstring[i] != '\n')&&(cmdstring[i] != '\t')) break;
				}
				if (i == cmdlen) break;
				start = i;
				continue;
			} else if (cmdstring[i] == '\'') {
				if (level) {
					if (((i+1) != cmdlen) && (cmdstring[i+1] != '\'')) {
						level = 0;
					} else {
						i++;
					}
				} else {
					level = 1;
				}
			}
			i++;
		}
	}
	if (dbdata->out_sqlda->sqld != 0) {
		Tcl_Obj *dbcmd = Tcl_NewObj();
		Tcl_GetCommandFullName(interp, dbdata->token, dbcmd);
		error = isc_dsql_set_cursor_name(status_vector, stmt, Tcl_GetStringFromObj(dbcmd,NULL), (short)NULL);
		Tcl_DecrRefCount(dbcmd);
		if (error) {
			Tcl_AppendResult(interp,"Interbase error:\n", NULL);
			dbi_Interbase_Error(interp,dbdata);
			goto error;
		}
		if (!usefetch) {
			error = dbi_Interbase_ToResult(interp,dbdata,dbdata->out_sqlda,nullvalue);
			if (error) {goto error;}
			if (dbi_Interbase_Transaction_Commit(interp,dbdata) != TCL_OK) {
				Tcl_DecrRefCount(nullvalue);
				return TCL_ERROR;
			}
		} else {
			dbdata->tuple = -1;
			dbdata->ntuples = -1;
		}
	} else {
		if (dbi_Interbase_Transaction_Commit(interp,dbdata) != TCL_OK) {
			Tcl_DecrRefCount(nullvalue);
			return TCL_ERROR;
		}
	}
	Tcl_DecrRefCount(nullvalue);
	return TCL_OK;
	error:
		Tcl_DecrRefCount(nullvalue);
		dbi_Interbase_Transaction_Rollback(interp,dbdata);
		return TCL_ERROR;
}

int dbi_Interbase_Fetch(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	Tcl_Obj *tuple = NULL, *field = NULL, *nullvalue = NULL;
	int fetch_option;
    static char *subCmds[] = {
		 "data", "lines", "pos", "fields", "clear", "isnull",
		(char *) NULL};
    enum ISubCmdIdx {
		Data, Lines, Pos, Fields, Clear, Isnull
    };
    static char *switches[] = {
		"-nullvalue",
		(char *) NULL};
    enum switchesIdx {
		Nullvalue
    };
	int ituple = -1, ifield = -1;
	ISC_STATUS *status_vector = dbdata->status;
	isc_stmt_handle *stmt = &(dbdata->stmt);
	Tcl_Obj *line = NULL, *element = NULL;
    long fetch_stat;
	int error;
	int nfields = dbdata->out_sqlda->sqld;
	if (dbdata->out_sqlda->sqld == 0) {
		Tcl_AppendResult(interp, "no result available: invoke exec method with -usefetch option first", NULL);
		return TCL_ERROR;
	}
	/*
	 * decode options
	 * --------------
	 */
	fetch_option = Data;
	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "?option? ?options? ?line? ?field?");
		return TCL_ERROR;
	} else if (objc == 2) {
		fetch_option = Data;
	} else {
		int i,index;
		error = Tcl_GetIndexFromObj(interp, objv[2], subCmds, "option", 0, (int *) &fetch_option);
		if (error != TCL_OK) {
			fetch_option = Data;
			i = 2;
		} else {
			Tcl_ResetResult(interp);
			i = 3;
		}
		for (; i < objc; i++) {
			if (Tcl_GetIndexFromObj(interp, objv[i], switches, "option", 0, &index)!= TCL_OK) {
				Tcl_ResetResult(interp);
				break;
			}
			switch (index) {
			case Nullvalue:
				i++;
				if (i == objc) {
					Tcl_AppendResult(interp,"no value given for option \"-nullvalue\"",NULL);
					return TCL_ERROR;
				}
				nullvalue = objv[i];
				break;
			}
		}
		if (i == objc) {
		} else if (i == (objc-1)) {
			tuple = objv[i];
		} else if (i == (objc-2)) {
			tuple = objv[i];
			field = objv[i+1];
		} else {
			Tcl_WrongNumArgs(interp, 2, objv, "?option? ?options? ?line? ?field?");
			return TCL_ERROR;
		}
		if (tuple != NULL) {
			char *string;
			int len;
			string = Tcl_GetStringFromObj(tuple,&len);
			if ((len == 7) && (strncmp(string,"current",7) == 0)) {
				ituple = dbdata->tuple;
			} else {
				error = Tcl_GetIntFromObj(interp,tuple,&ituple);
				if (error) {return TCL_ERROR;}
			}
			if (ituple < 0) {
				Tcl_Obj *buffer;
				buffer = Tcl_NewIntObj(ituple);
				Tcl_AppendResult(interp, "line ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
				Tcl_DecrRefCount(buffer);
				return TCL_ERROR;
			}
		} else {
			ituple = -1;
		}
		if (field != NULL) {
			error = Tcl_GetIntFromObj(interp,field,&ifield);
			if (error) {return TCL_ERROR;}
		} else {
			ifield = -1;
		}
	}
	/*
	 * result info
	 * -----------
	 */
	switch (fetch_option) {
		case Lines:
			Tcl_AppendResult(interp,"dbi_interbase: fetch lines not supported", NULL);
			return TCL_OK;
		case Fields:
			error = dbi_Interbase_Fetch_Fields(interp,dbdata,dbdata->out_sqlda,&line);
			if (error) {goto error;}
			Tcl_SetObjResult(interp, line);
			return TCL_OK;
		case Clear:
			dbi_Interbase_Free_out_sqlda(dbdata);
			error = isc_dsql_free_statement(status_vector, stmt, DSQL_close);
			dbdata->cursor_open = 0;
			return TCL_OK;
		case Pos:
			Tcl_SetObjResult(interp,Tcl_NewIntObj(dbdata->tuple));
			return TCL_OK;
	}
	/*
	 * fetch data
	 * ----------
	 */
	if (ituple == -1) {
		ituple = dbdata->tuple+1;
	}
	if (dbdata->tuple == -1) {
		/* get to the first line in the result, if this is the first call to fetch */
		fetch_stat = isc_dsql_fetch(status_vector, stmt, SQL_DIALECT_V6, dbdata->out_sqlda);
		if (fetch_stat) {dbdata->ntuples = dbdata->tuple;goto out_of_position;}
		dbdata->tuple++;		
	}
	if ((dbdata->ntuples != -1)&&(ituple > dbdata->ntuples)) {
		goto out_of_position;
	}
	if (ifield >= nfields) {
		Tcl_Obj *buffer;
		buffer = Tcl_NewIntObj(ifield);
		Tcl_AppendResult(interp, "field ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
		Tcl_DecrRefCount(buffer);
		return TCL_ERROR;
	}
	if ((dbdata->ntuples != -1)&&(ituple >= dbdata->ntuples)) {
		goto out_of_position;
	}
	/* move to the requested line */
	if (ituple != dbdata->tuple) {
		if (ituple < dbdata->tuple) {
			Tcl_AppendResult(interp,"interbase error: backwards positioning for fetch not supported",NULL);
			return TCL_ERROR;
		}
		while (dbdata->tuple < ituple) {
			fetch_stat = isc_dsql_fetch(status_vector, stmt, SQL_DIALECT_V6, dbdata->out_sqlda);
			if (fetch_stat) {dbdata->ntuples = dbdata->tuple;goto out_of_position;}
			dbdata->tuple++;		
		}
	}
	if (nullvalue == NULL) {
		nullvalue = Tcl_NewObj();
	}
	Tcl_IncrRefCount(nullvalue);
	switch (fetch_option) {
		case Data:
			if (ifield == -1) {
				error = dbi_Interbase_Fetch_Row(interp,dbdata,dbdata->out_sqlda,nullvalue,&line);
				if (error) {return error;}
				Tcl_SetObjResult(interp, line);
			} else {
				error = dbi_Interbase_Fetch_One(interp,dbdata,ifield,&element);
				if (error) {goto error;}
				if (element == NULL) {
					Tcl_SetObjResult(interp, nullvalue);
				} else {
					Tcl_SetObjResult(interp, element);
				}
			}
			Tcl_DecrRefCount(nullvalue);
			break;
		case Isnull:
			error = dbi_Interbase_Fetch_One(interp,dbdata,ifield,&element);
			if (error) {goto error;}
			if (element == NULL) {
				Tcl_SetObjResult(interp,Tcl_NewIntObj(1));
			} else {
				Tcl_SetObjResult(interp,Tcl_NewIntObj(0));
			}
			return TCL_OK;
	}
	return TCL_OK;
	error:
		if (nullvalue != NULL) {Tcl_DecrRefCount(nullvalue);}
		return TCL_ERROR;
	out_of_position:
		{
		Tcl_Obj *buffer;
		buffer = Tcl_NewIntObj(dbdata->tuple+1);
		Tcl_AppendResult(interp, "line ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
		Tcl_DecrRefCount(buffer);
		return TCL_ERROR;
		}
}


int dbi_Interbase_Supports(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	Tcl_Obj *keyword)
{
	static char *keywords[] = {
		"columnperm","blobparams","roles","domains",
		(char *) NULL};
	enum keywordsIdx {
		Columnperm, Roles, Domains, Blobparams,
	};
	int error,index;
	if (keyword == NULL) {
		char **keyword = keywords;
		int index = 0;
		while (1) {
			if (*keyword == NULL) break;
			switch(index) {
				default:
					Tcl_AppendElement(interp,*keyword);
			}
			keyword++;
			index++;
		}
	} else {
		error = Tcl_GetIndexFromObj(interp, keyword, keywords, "", 0, (int *) &index);
		if (error == TCL_OK) {
			switch(index) {
				default:
					Tcl_SetObjResult(interp,Tcl_NewIntObj(1));
			}
		} else {
			Tcl_SetObjResult(interp,Tcl_NewIntObj(0));
		}
	}
	return TCL_OK;
}

int dbi_Interbase_Info(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	int objc,
	Tcl_Obj **objv)
{
	Tcl_Obj *cmd,*dbcmd;
	int error,i;
	dbcmd = Tcl_NewObj();
	Tcl_GetCommandFullName(interp, dbdata->token, dbcmd);
	cmd = Tcl_NewStringObj("::dbi::interbase::info",-1);
	Tcl_IncrRefCount(cmd);
	error = Tcl_ListObjAppendElement(interp,cmd,dbcmd);
	if (error) {Tcl_DecrRefCount(cmd);Tcl_DecrRefCount(dbcmd);return error;}
	for (i = 0 ; i < objc ; i++) {
		error = Tcl_ListObjAppendElement(interp,cmd,objv[i]);
		if (error) {Tcl_DecrRefCount(cmd);return error;}
	}
	error = Tcl_EvalObj(interp,cmd);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Interbase_Tables(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata)
{
	int error;
	Tcl_Obj *cmd = Tcl_NewStringObj("select rdb$relation_name from rdb$relations where RDB$SYSTEM_FLAG = 0",-1);
	error = dbi_Interbase_Exec(interp,dbdata,cmd,0,NULL,0,NULL);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Interbase_Tableinfo(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	Tcl_Obj *table,
	Tcl_Obj *varName,
	char *cmdstring)
{
	Tcl_Obj *cmd,*dbcmd;
	int error;
	dbcmd = Tcl_NewObj();
	Tcl_GetCommandFullName(interp, dbdata->token, dbcmd);
	cmd = Tcl_NewStringObj(cmdstring,-1);
	Tcl_IncrRefCount(cmd);
	error = Tcl_ListObjAppendElement(interp,cmd,dbcmd);
	if (error) {Tcl_DecrRefCount(cmd);Tcl_DecrRefCount(dbcmd);return error;}
	error = Tcl_ListObjAppendElement(interp,cmd,table);
	if (error) {Tcl_DecrRefCount(cmd);return error;}
	if (varName != NULL) {
		error = Tcl_ListObjAppendElement(interp,cmd,varName);
		if (error) {Tcl_DecrRefCount(cmd);return error;}
	}
	error = Tcl_EvalObj(interp,cmd);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Interbase_Serial(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	Tcl_Obj *subcmd,
	Tcl_Obj *table,
	Tcl_Obj *field,
	Tcl_Obj *current)
{
	Tcl_Obj *cmd,*dbcmd;
	int error,index;
    static char *subCmds[] = {
		"add", "delete", "set", "next",
		(char *) NULL};
    enum ISubCmdIdx {
		Add, Delete, Set, Next
    };
	error = Tcl_GetIndexFromObj(interp, subcmd, subCmds, "suboption", 0, (int *) &index);
	if (error != TCL_OK) {
		return error;
	}
    switch (index) {
	    case Add:
			cmd = Tcl_NewStringObj("::dbi::interbase::serial_add",-1);
			break;
		case Delete:
			cmd = Tcl_NewStringObj("::dbi::interbase::serial_delete",-1);
			current = NULL;
			break;
		case Set:
			cmd = Tcl_NewStringObj("::dbi::interbase::serial_set",-1);
			break;
		case Next:
			cmd = Tcl_NewStringObj("::dbi::interbase::serial_next",-1);
			break;
	}
	Tcl_IncrRefCount(cmd);
	dbcmd = Tcl_NewObj();
	Tcl_GetCommandFullName(interp, dbdata->token, dbcmd);
	error = Tcl_ListObjAppendElement(interp,cmd,dbcmd);
	if (error) {Tcl_DecrRefCount(cmd);Tcl_DecrRefCount(dbcmd);return error;}
	error = Tcl_ListObjAppendElement(interp,cmd,table);
	if (error) {Tcl_DecrRefCount(cmd);return error;}
	error = Tcl_ListObjAppendElement(interp,cmd,field);
	if (error) {Tcl_DecrRefCount(cmd);return error;}
	if (current != NULL) {
		error = Tcl_ListObjAppendElement(interp,cmd,current);
		if (error) {Tcl_DecrRefCount(cmd);return error;}
	}
	error = Tcl_EvalObj(interp,cmd);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Interbase_Createdb(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	static char *switches[] = {"-user","-password","-pagesize","-extra", (char *) NULL};
	ISC_STATUS *status_vector = dbdata->status;
	Tcl_Obj *cmd = NULL;
	char *string=NULL;
	int len,i,index,error;
	if (dbdata->db != NULL) {
		Tcl_AppendResult(interp,"dbi object has open connection, close first", NULL);
		goto error;
	}
	if (objc < 3) {
		Tcl_WrongNumArgs(interp,2,objv,"database ?options?");
		return TCL_ERROR;
	}
	cmd = Tcl_NewStringObj("create database '",-1);
	Tcl_AppendObjToObj(cmd,objv[2]);
	Tcl_AppendToObj(cmd,"'",1);
	for (i = 3; i < objc; i++) {
		if (Tcl_GetIndexFromObj(interp, objv[i], switches, "option", 0, &index)!= TCL_OK) {
			return TCL_ERROR;
		}
		switch (index) {
			case 0:			/* -user */
				if (i == (objc)) {
					Tcl_AppendResult(interp,"\"-user\" option must be followed by the username",NULL);
					return TCL_ERROR;
				}
				Tcl_AppendToObj(cmd," user '",-1);
				Tcl_AppendObjToObj(cmd,objv[i+1]);
				Tcl_AppendToObj(cmd,"'",1);
				i++;
				break;
			case 1:			/* -password */
				if (i == (objc-1)) {
					Tcl_AppendResult(interp,"\"-password\" option must be followed by the password of the user",NULL);
					return TCL_ERROR;
				}
				Tcl_AppendToObj(cmd," password '",-1);
				Tcl_AppendObjToObj(cmd,objv[i+1]);
				Tcl_AppendToObj(cmd,"'",1);
				i++;
				break;
			case 2:			/* -pagesize */
				if (i == (objc-1)) {
					Tcl_AppendResult(interp,"\"-pagesize\" option must be followed by the pagesize",NULL);
					return TCL_ERROR;
				}
				Tcl_AppendToObj(cmd," page_size ",-1);
				Tcl_AppendObjToObj(cmd,objv[i+1]);
				i++;
				break;
			case 3:			/* -extra */
				if (i == (objc-1)) {
					Tcl_AppendResult(interp,"\"-extra\" option must be followed by the pagesize",NULL);
					return TCL_ERROR;
				}
				Tcl_AppendToObj(cmd," ",-1);
				Tcl_AppendObjToObj(cmd,objv[i+1]);
				i++;
				break;
		}
	}
	string = Tcl_GetStringFromObj(cmd,&len);
	error = isc_dsql_execute_immediate(status_vector, &(dbdata->db), dbi_Interbase_trans(dbdata),
		0,string, SQL_DIALECT_V6, NULL);
	if (error) {
		Tcl_AppendResult(interp,"creation of database \"",
			Tcl_GetStringFromObj(objv[2],NULL) , "\" failed:\n", NULL);
		dbi_Interbase_Error(interp,dbdata);
		goto error;
	}
	isc_detach_database(status_vector, &(dbdata->db));
	Tcl_DecrRefCount(cmd);
	return TCL_OK;
	error:
		if (cmd != NULL) {Tcl_DecrRefCount(cmd);}
		return TCL_ERROR;
}

int dbi_Interbase_Close(
	dbi_Interbase_Data *dbdata)
{
	ISC_STATUS *status_vector = dbdata->status;
	isc_stmt_handle *stmt = &(dbdata->stmt);
	int error;
	dbi_Interbase_Free_out_sqlda(dbdata);
	if (dbdata->stmt != NULL) {
		isc_dsql_free_statement(status_vector, stmt, DSQL_drop);
		dbdata->stmt = NULL;
	}
	error = dbi_Interbase_Transaction_Rollback(dbdata->interp,dbdata);
	if (error) {return error;}
	while (dbdata->clonesnum) {
		Tcl_DeleteCommandFromToken(dbdata->interp,dbdata->clones[0]->token);
	}
	if (dbdata->clones != NULL) {
		Tcl_Free((char *)dbdata->clones);
		dbdata->clones = NULL;
	}
	if (dbdata->parent == NULL) {
		if (dbdata->trans != NULL) {
			isc_commit_transaction(status_vector, &(dbdata->trans));
			dbdata->trans = NULL;
		}
		if (dbdata->db != NULL) {
			if (isc_detach_database(dbdata->status, &(dbdata->db))) {
				Tcl_AppendResult(dbdata->interp,"error closing connection:\n", NULL);
				dbi_Interbase_Error(dbdata->interp,dbdata);
				return TCL_ERROR;
			}
			dbdata->db = NULL;
		}
		if (dbdata->database != NULL) {
			Tcl_DecrRefCount(dbdata->database);dbdata->database = NULL;
		}
		if (dbdata->dpb != NULL) {
			Tcl_Free(dbdata->dpb);
			dbdata->dpblen = 0;
			dbdata->dpb = NULL;
		}
	} else {
		dbi_Interbase_Data *parent = dbdata->parent;
		int i;
		for (i = 0 ; i < parent->clonesnum; i++) {
			if (parent->clones[i] == dbdata) break;
		}
		i++;
		for (; i < parent->clonesnum; i++) {
			parent->clones[i-1] = parent->clones[i];
		}
		parent->clonesnum--;
	}
	return TCL_OK;
}

int dbi_Interbase_Dropdb(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata)
{
	int error;
	if (dbdata->db == NULL) {
		Tcl_AppendResult(interp,"dbi object has no open connection", NULL);
		return TCL_ERROR;
	}
	error = isc_drop_database(dbdata->status,&(dbdata->db));
	if (error) {
		dbi_Interbase_Error(interp,dbdata);
		return TCL_ERROR;
	}
	error = dbi_Interbase_Close(dbdata);
	if (error) {return TCL_ERROR;}
	return TCL_OK;
}

int dbi_Interbase_Destroy(
	ClientData clientdata)
{
	dbi_Interbase_Data *dbdata = (dbi_Interbase_Data *)clientdata;
	dbi_Interbase_Close(dbdata);
	Tcl_Free((char *)dbdata);
	Tcl_DeleteExitHandler((Tcl_ExitProc *)dbi_Interbase_Destroy, clientdata);
	return TCL_OK;
}

int dbi_Interbase_Interface(
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	int i;
    static char *interfaces[] = {
		"dbi", "0.1", "dbi_admin", "0.1",
		(char *) NULL};
	if ((objc < 2)||(objc > 3)) {
		Tcl_WrongNumArgs(interp,2,objv,"?pattern?");
		return TCL_ERROR;
	}
	Tcl_ResetResult(interp);
	if (objc == 2) {
		i = 0;
		while (interfaces[i] != NULL) {
			Tcl_AppendResult(interp, interfaces[i], " ", interfaces[i+1], " ", (char *) NULL);
			i+=2;
		}
		return TCL_OK;
	} else {
		char *interface;
		int len;
		interface = Tcl_GetStringFromObj(objv[2],&len);
		i = 0;
		while (interfaces[i] != NULL) {
			if ((strlen(interfaces[i]) == len) && (strncmp(interfaces[i],interface,len) == 0)) {
				Tcl_AppendResult(interp,interfaces[i+1]);
				return TCL_OK;
			}
			i+=2;
		}
		Tcl_AppendResult(interp, Tcl_GetStringFromObj(objv[0],NULL), 
			" does not support interface ", interface, NULL);
		return TCL_ERROR;
	}
}

int Dbi_interbase_DbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_Interbase_Data *dbdata = (dbi_Interbase_Data *)clientdata;
	int error=TCL_OK,i,index;
    static char *subCmds[] = {
		"interface","open", "exec", "fetch", "close",
		"info", "tables","fields",
		"begin", "commit", "rollback",
		"destroy", "serial","supports",
		"create", "drop","clone","clones","parent",
		(char *) NULL};
    enum ISubCmdIdx {
		Interface, Open, Exec, Fetch, Close,
		Info, Tables, Fields,
		Begin, Commit, Rollback,
		Destroy, Serial, Supports,
		Create, Drop, Clone, Clones, Parent
    };
	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "option ?...?");
		return TCL_ERROR;
	}
	error = Tcl_GetIndexFromObj(interp, objv[1], subCmds, "option", 0, (int *) &index);
	if (error != TCL_OK) {
		return error;
	}
    switch (index) {
    case Interface:
		return dbi_Interbase_Interface(interp,objc,objv);
    case Open:
		if (dbdata->parent != NULL) {
			Tcl_AppendResult(interp,"clone may not use open",NULL);
			return TCL_ERROR;
		}
		return dbi_Interbase_Open(interp,dbdata,objc,objv);
	case Create:
		return dbi_Interbase_Createdb(interp,dbdata,objc,objv);
	case Destroy:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		Tcl_DeleteCommandFromToken(interp,dbdata->token);
		return TCL_OK;
	case Clones:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->parent != NULL) {
			Tcl_AppendResult(interp,"error: object \"",Tcl_GetStringFromObj(objv[0],NULL),"\" is a clone", NULL);
			return TCL_ERROR;
		}
		if (dbdata->clonesnum) {
			Tcl_Obj *dbcmd,*result;
			int i;
			result = Tcl_NewObj();
			for (i = 0 ; i < dbdata->clonesnum; i++) {
				dbcmd = Tcl_NewObj();
				Tcl_GetCommandFullName(interp, dbdata->clones[i]->token, dbcmd);
				if (strncmp(Tcl_GetStringFromObj(dbcmd,NULL),"::dbi::interbase::priv_",23) == 0) continue;
				error = Tcl_ListObjAppendElement(interp,result,dbcmd);
				if (error) {Tcl_DecrRefCount(result);Tcl_DecrRefCount(dbcmd);return error;}
			}
			Tcl_SetObjResult(interp,result);
		}
		return TCL_OK;
	case Parent:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->parent != NULL) {
			dbdata = dbdata->parent;
		}
		{
		Tcl_Obj *result;
		result = Tcl_NewObj();
		Tcl_GetCommandFullName(interp, dbdata->token, result);
		Tcl_SetObjResult(interp,result);
		return TCL_OK;
		}
	}
	/* commands only allowable with open database */
	if (dbdata->db == NULL) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		return TCL_ERROR;
	}
    switch (index) {
	case Exec:
		{
		Tcl_Obj *nullvalue = NULL, *command = NULL;
		char *string;
		int usefetch = 0;
		int stringlen;
		if (objc < 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "?options? command ?argument? ?...?");
			return TCL_ERROR;
		}
		i = 2;
		while (i < objc) {
			string = Tcl_GetStringFromObj(objv[i],&stringlen);
			if (string[0] != '-') break;
			if ((stringlen==9)&&(strncmp(string,"-usefetch",9)==0)) {
				usefetch = 1;
			} else if ((stringlen==8)&&(strncmp(string,"-command",8)==0)) {
				i++;
				if (i == objc) {
					Tcl_AppendResult(interp,"no value given for option \"-command\"",NULL);
					return TCL_ERROR;
				}
				Tcl_AppendResult(interp,"dbi_interbase: -command not supported",NULL);
				return TCL_ERROR;
				command = objv[i];
			} else if ((stringlen==10)&&(strncmp(string,"-nullvalue",10)==0)) {
				i++;
				if (i == objc) {
					Tcl_AppendResult(interp,"no value given for option \"-nullvalue\"",NULL);
					return TCL_ERROR;
				}
				nullvalue = objv[i];
			} else {
				Tcl_AppendResult(interp,"unknown option \"",string,"\", must be one of: -usefetch, -nullvalue",NULL);
				return TCL_ERROR;
			}
			i++;
		}
		if (command != NULL) {
			usefetch = 1;
		}
		return dbi_Interbase_Exec(interp,dbdata,objv[i],usefetch,nullvalue,objc-i-1,objv+i+1);
		}
	case Fetch:
		return dbi_Interbase_Fetch(interp,dbdata, objc, objv);
	case Info:
		return dbi_Interbase_Info(interp,dbdata,objc-2,objv+2);
	case Tables:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_Interbase_Tables(interp,dbdata);
	case Fields:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "tablename");
			return TCL_ERROR;
		}
		return dbi_Interbase_Tableinfo(interp,dbdata,objv[2],NULL,"::dbi::interbase::fieldsinfo");
	case Close:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->parent == NULL) {
			return dbi_Interbase_Close(dbdata);
		} else {
			/* closing a clone also destroys it */
			Tcl_DeleteCommandFromToken(interp,dbdata->token);
			return TCL_OK;
		}
	case Begin:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->db == NULL) {
			Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
			return error;
		}
		error = dbi_Interbase_Transaction_Start(interp,dbdata);
		if (error) {dbi_Interbase_Error(interp,dbdata);return error;}
		dbi_Interbase_autocommit(dbdata,0);
		return TCL_OK;
	case Commit:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->db == NULL) {
			Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
			return error;
		}
		dbi_Interbase_autocommit(dbdata,1);
		error = dbi_Interbase_Transaction_Commit(interp,dbdata);
		if (error) {dbi_Interbase_Error(interp,dbdata);return error;}
		return TCL_OK;
	case Rollback:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->db == NULL) {
			Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
			return error;
		}
		dbi_Interbase_autocommit(dbdata,1);
		error = dbi_Interbase_Transaction_Rollback(interp,dbdata);
		if (error) {dbi_Interbase_Error(interp,dbdata);return error;}
		return TCL_OK;
	case Serial:
		if (objc < 5) {
			Tcl_WrongNumArgs(interp, 2, objv, "option table field ?value?");
			return TCL_ERROR;
		}
		if (objc == 6) {
			return dbi_Interbase_Serial(interp,dbdata,objv[2],objv[3],objv[4],objv[5]);
		} else {
			return dbi_Interbase_Serial(interp,dbdata,objv[2],objv[3],objv[4],NULL);
		}
	case Drop:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_Interbase_Dropdb(interp,dbdata);
	case Clone:
		if ((objc != 2) && (objc != 3)) {
			Tcl_WrongNumArgs(interp, 2, objv, "?name?");
			return TCL_ERROR;
		}
		if (objc == 3) {
			return Dbi_interbase_Clone(interp,dbdata,objv[2]);
		} else {
			return Dbi_interbase_Clone(interp,dbdata,NULL);
		}
	case Supports:
		if (objc == 2) {
			return dbi_Interbase_Supports(interp,dbdata,NULL);
		} if (objc == 3) {
			return dbi_Interbase_Supports(interp,dbdata,objv[2]);
		} else {
			Tcl_WrongNumArgs(interp, 2, objv, "?keyword?");
			return TCL_ERROR;
		}
	}
	return error;
}

static int dbi_num = 0;
int Dbi_interbase_DoNewDbObjCmd(
	dbi_Interbase_Data *dbdata,
	Tcl_Interp *interp,
	Tcl_Obj *dbi_nameObj)
{
	char buffer[40];
	char *dbi_name;
	dbdata->interp = interp;
	dbdata->db = NULL;
	dbdata->stmt = NULL;
	dbdata->trans = NULL;
	dbdata->autocommit = 1;
	dbdata->dpblen = 0;
	dbdata->database=NULL;
	dbdata->dpb = NULL;
	dbdata->cursor_open = 0;
	dbdata->clones = NULL;
	dbdata->clonesnum = 0;
	dbdata->parent = NULL;
	/* 
	 * Allocate enough space for 20 fields.  
	 * If more fields get selected, re-allocate SQLDA later.
	 */
	dbdata->out_sqlda = (XSQLDA ISC_FAR *) Tcl_Alloc(XSQLDA_LENGTH(20));
	dbdata->out_sqlda->sqln = 20;
	dbdata->out_sqlda->sqld = 0;
	dbdata->out_sqlda->version = SQLDA_VERSION1;
	dbdata->out_sqlda_filled = 0;
	dbdata->in_sqlda = (XSQLDA ISC_FAR *) Tcl_Alloc(XSQLDA_LENGTH(20));
	dbdata->in_sqlda->sqln = 20;
	dbdata->in_sqlda->sqld = 0;
	dbdata->in_sqlda->version = SQLDA_VERSION1;
	if (dbi_nameObj == NULL) {
		dbi_num++;
		sprintf(buffer,"::dbi::interbase::dbi%d",dbi_num);
		dbi_nameObj = Tcl_NewStringObj(buffer,strlen(buffer));
	}
	dbi_name = Tcl_GetStringFromObj(dbi_nameObj,NULL);
	dbdata->token = Tcl_CreateObjCommand(interp,dbi_name,(Tcl_ObjCmdProc *)Dbi_interbase_DbObjCmd,
		(ClientData)dbdata,(Tcl_CmdDeleteProc *)dbi_Interbase_Destroy);
	Tcl_CreateExitHandler((Tcl_ExitProc *)dbi_Interbase_Destroy, (ClientData)dbdata);
	Tcl_SetObjResult(interp,dbi_nameObj);
	return TCL_OK;
}

int Dbi_interbase_NewDbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_Interbase_Data *dbdata;
	int error;
	if ((objc < 1)||(objc > 2)) {
		Tcl_WrongNumArgs(interp,2,objv,"?dbName?");
		return TCL_ERROR;
	}
	dbdata = (dbi_Interbase_Data *)Tcl_Alloc(sizeof(dbi_Interbase_Data));
	if (objc == 2) {
		error = Dbi_interbase_DoNewDbObjCmd(dbdata,interp,objv[1]);
	} else {
		error = Dbi_interbase_DoNewDbObjCmd(dbdata,interp,NULL);
	}
	if (error) {
		Tcl_Free((char *)dbdata);
	}
	return error;
}

int Dbi_interbase_Clone(
	Tcl_Interp *interp,
	dbi_Interbase_Data *dbdata,
	Tcl_Obj *name)
{
	dbi_Interbase_Data *parent = NULL;
	dbi_Interbase_Data *clone_dbdata = NULL;
	int error;
	parent = dbdata;
	while (parent->parent != NULL) {parent = parent->parent;}
	clone_dbdata = (dbi_Interbase_Data *)Tcl_Alloc(sizeof(dbi_Interbase_Data));
	error = Dbi_interbase_DoNewDbObjCmd(clone_dbdata,interp,name);
	if (error) {Tcl_Free((char *)clone_dbdata);return TCL_ERROR;}
	name = Tcl_GetObjResult(interp);
	parent->clonesnum++;
	parent->clones = (dbi_Interbase_Data **)Tcl_Realloc((char *)parent->clones,parent->clonesnum*sizeof(dbi_Interbase_Data **));
	parent->clones[parent->clonesnum-1] = clone_dbdata;
	clone_dbdata->parent = parent;
	clone_dbdata->dpblen = parent->dpblen;
	clone_dbdata->dpb = parent->dpb;
	clone_dbdata->database = parent->database;
	clone_dbdata->dpbpos = parent->dpbpos;
	clone_dbdata->db = parent->db;
	error = isc_dsql_allocate_statement(clone_dbdata->status, &(clone_dbdata->db), &(clone_dbdata->stmt));
	if (error) {
		Tcl_AppendResult(interp,"Interbase error on statement allocation:\n", NULL);
		dbi_Interbase_Error(interp,dbdata);
		goto error;
	}
	return TCL_OK;
	error:
		Tcl_DeleteCommandFromToken(interp,clone_dbdata->token);
		Tcl_Free((char *)clone_dbdata);
		return TCL_ERROR;
}

int Dbi_interbase_Init(interp)
	Tcl_Interp *interp;		/* Interpreter to add extra commands */
{
#ifdef USE_TCL_STUBS
	if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
		return TCL_ERROR;
	}
#endif
	Tcl_CreateObjCommand(interp,"dbi_interbase",(Tcl_ObjCmdProc *)Dbi_interbase_NewDbObjCmd,
		(ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
	Tcl_Eval(interp,"");
	return TCL_OK;
}
