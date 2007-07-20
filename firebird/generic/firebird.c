/*
 *       File:    firebird.c
 *       Purpose: dbi extension to Tcl: firebird backend
 *       Author:  Copyright (c) 1998 Peter De Rijk
 *
 *       See the file "README" for information on usage and redistribution
 *       of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "firebird.h"
#include "tcl.h"

#define V2 1

#define QUAD_HIGH gds_quad_high
#define QUAD_LOW gds_quad_low
typedef struct vary VARY;

int Dbi_firebird_Clone(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	Tcl_Obj *name);

int dbi_Firebird_Free_Stmt(
	dbi_Firebird_Data *dbdata,
	int action);

/******************************************************************/

int Dbi_firebird_initsqlda(XSQLDA **sqldaPtr) {
	XSQLDA *sqlda;
	sqlda = (XSQLDA ISC_FAR *) Tcl_Alloc(XSQLDA_LENGTH(20));
	sqlda->sqln = 20;
	sqlda->sqld = 0;
	sqlda->version = SQLDA_VERSION1;
	*sqldaPtr = sqlda;
}

int dbi_Firebird_autocommit_state(
	dbi_Firebird_Data *dbdata)
{
	if (dbdata->parent != NULL) {
		return dbdata->parent->autocommit;
	} else {
		return dbdata->autocommit;
	}
}

int dbi_Firebird_autocommit_set(
	dbi_Firebird_Data *dbdata,
	int state)
{
	if (dbdata->parent != NULL) {
		dbdata->parent->autocommit = state;
	} else {
		dbdata->autocommit = state;
	}
	return state;
}

isc_tr_handle *dbi_Firebird_trans(
	dbi_Firebird_Data *dbdata)
{
	if (dbdata->parent != NULL) {
		return &(dbdata->parent->trans);
	} else {
		return &(dbdata->trans);
	}
}

int dbi_Firebird_trans_state(
	dbi_Firebird_Data *dbdata)
{
	if (dbdata->parent != NULL) {
		if (dbdata->parent->trans == NULL) {return 0;} else {return 1;}
	} else {
		if (dbdata->trans == NULL) {return 0;} else {return 1;}
	}
}

int dbi_Firebird_String_Tolower(
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

int dbi_Firebird_Reconnect(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata)
{
	char *string=NULL;
	int len,error;
	if (dbdata->db == NULL) {return TCL_ERROR;}
	dbi_Firebird_Free_Stmt(dbdata,DSQL_close);
	error = isc_detach_database(dbdata->status, &(dbdata->db));
	dbdata->db = NULL;
	dbdata->stmt = NULL;
	dbdata->trans = NULL;
	dbdata->cursor_open = 0;
	string = Tcl_GetStringFromObj(dbdata->database,&len);
	error = isc_attach_database(dbdata->status, len, string, &(dbdata->db), dbdata->dpbpos, dbdata->dpb);
	if (error) {
		ISC_STATUS *status_vector = dbdata->status;
		long SQLCODE;
		Tcl_AppendResult(interp,"reconnection to database \"",
			Tcl_GetStringFromObj(dbdata->database,NULL) , "\" failed: ", NULL);
		if (status_vector[0] == 1 && status_vector[1]) {
			char msg[512];
			SQLCODE = isc_sqlcode(status_vector);
			isc_sql_interprete(SQLCODE, msg, 512);
			while(isc_interprete(msg + 1, &status_vector)) {
				Tcl_AppendResult(interp,msg+1," - ", NULL);
			}
		}
		
		goto error;
	}
	error = isc_dsql_allocate_statement(dbdata->status, &(dbdata->db), &(dbdata->stmt));
	if (error) {
		Tcl_AppendResult(interp,"Firebird error on statement allocation:\n", NULL);
		goto error;
	}
	if (dbdata->clonesnum) {
		dbi_Firebird_Data *clone_dbdata;
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
				Tcl_AppendResult(interp,"Firebird error on statement allocation:\n", NULL);
				goto error;
			}
		}
	}
	return TCL_OK;
	error:
		return TCL_ERROR;
}

int dbi_Firebird_TclEval(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	char *cmdstring,
	int objc,
	Tcl_Obj **objv)
{
	Tcl_Obj *cmd,*dbcmd;
	int error,i;
	dbcmd = Tcl_NewObj();
	Tcl_GetCommandFullName(interp, dbdata->token, dbcmd);
	cmd = Tcl_NewStringObj(cmdstring,-1);
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

int dbi_Firebird_Error(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	char *premsg)
{
	ISC_STATUS *status_vector = dbdata->status;
	Tcl_Obj *errormsg;
	long SQLCODE = 0;
	int error;
	if (strlen(premsg) != 0) {
		Tcl_AppendResult(interp,"error ",premsg,":\n", NULL);
	}
	if (status_vector[0] == 1 && status_vector[1]) {
		char msg[512];
		SQLCODE = isc_sqlcode(status_vector);
		isc_sql_interprete(SQLCODE, msg, 512);
		errormsg = Tcl_NewObj();
		while(isc_interprete(msg + 1, &status_vector)) {
			Tcl_AppendStringsToObj(errormsg,msg+1," - ", NULL);
		}
	}
	if ((dbi_Firebird_autocommit_state(dbdata))&&(dbi_Firebird_trans_state(dbdata))) {
/*fprintf(stdout,"error rollback transaction %X for stmt=%X\n",(uint)dbi_Firebird_trans(dbdata),(uint)dbdata->stmt);fflush(stdout);*/
		isc_tr_handle *trans = dbi_Firebird_trans(dbdata);
		if (*trans != NULL) {
			error = isc_rollback_transaction(status_vector, trans);
			if (dbdata->out_sqlda == NULL) {
				Dbi_firebird_initsqlda(&(dbdata->out_sqlda));
			}
		}
		if (error) {
			Tcl_AppendResult(interp,"error rolling back transaction:\n", NULL);
			dbi_Firebird_Error(interp,dbdata,"rollback in errorhandling");
			return TCL_ERROR;
		}
	}
	dbi_Firebird_TclEval(interp,dbdata,"::dbi::firebird::errorclean",1,&errormsg);	
	if (dbdata->cursor_open != 0) {dbi_Firebird_Free_Stmt(dbdata,DSQL_close);}
	if (SQLCODE == -902) {
		error = dbi_Firebird_Reconnect(interp,dbdata);
		if (error) {
			Tcl_AppendResult(interp,"error while reconnecting\n", NULL);
			return TCL_ERROR;
		} else {
			Tcl_AppendResult(interp,"reconnected\n", NULL);
			return TCL_OK;
		}
	}
	return SQLCODE;
}

int dbi_Firebird_Blob_Create(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
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
	error = isc_create_blob2(status_vector, &(dbdata->db), dbi_Firebird_trans(dbdata), &blob_handle, blob_id, 0, NULL);
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
		dbi_Firebird_Error(interp,dbdata,"creating blob");
		return TCL_ERROR;
}

int dbi_Firebird_Blob_Get(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	ISC_QUAD *blob_id,
	Tcl_Obj **resultPtr)
{
	ISC_STATUS *status_vector = dbdata->status;
	ISC_STATUS blob_stat = 0;
	isc_blob_handle blob_handle = NULL;
	Tcl_Obj *result = NULL;
	char blob_segment[8192];
	int error;
	unsigned short actual_seg_len;
	error = isc_open_blob2(status_vector, &(dbdata->db),  dbi_Firebird_trans(dbdata), &blob_handle, blob_id, 0, NULL);
	if (error) {dbi_Firebird_Error(interp,dbdata,"opening blob");goto error;}
	result = Tcl_NewStringObj("",0);
	actual_seg_len = 8192;
	while (actual_seg_len) {
		blob_stat = isc_get_segment(status_vector, &blob_handle,
			&actual_seg_len,8192,blob_segment);
		Tcl_AppendToObj(result,blob_segment,actual_seg_len);
		if (blob_stat == isc_segstr_eof) break;
		if (blob_stat) {dbi_Firebird_Error(interp,dbdata,"getting blob segment");goto error;}
	}
	if (isc_close_blob(status_vector, &blob_handle)) {
		dbi_Firebird_Error(interp,dbdata,"closing blob");
		goto error;
	}
	*resultPtr = result;
	return TCL_OK;
	error:
		if (result != NULL) {Tcl_DecrRefCount(result);}
		return TCL_ERROR;
}

int dbi_Firebird_Free_out_sqlda(
	XSQLDA *sqlda,
	int ipos)
{
	int i;
	if (sqlda == NULL) {return TCL_OK;}
	for(i = 0 ; i < ipos ; i++) {
		if (sqlda->sqlvar[i].sqldata != NULL) Tcl_Free((char *)sqlda->sqlvar[i].sqldata);
		if (sqlda->sqlvar[i].sqltype & 1) Tcl_Free((char *)sqlda->sqlvar[i].sqlind);
	}
	sqlda->sqld = 0;
	return TCL_OK;
}

int dbi_Firebird_Free_Stmt(
	dbi_Firebird_Data *dbdata,
	int action)
{
	ISC_STATUS *status_vector = dbdata->status;
	isc_stmt_handle *stmt = &(dbdata->stmt);
	int error;
	error = isc_dsql_free_statement(status_vector, stmt, action);
	dbdata->cursor_open = 0;
	if (dbdata->out_sqlda_cache == NULL) {
		if (dbdata->out_sqlda != NULL) {dbi_Firebird_Free_out_sqlda(dbdata->out_sqlda,dbdata->out_sqlda->sqld);}
	}
	return TCL_OK;
}

int dbi_Firebird_Prepare_out_sqlda(
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
				goto error;
		}
		if (var->sqltype & 1) { /* allocate variable to hold NULL status */ 
			var->sqlind = (short *)Tcl_Alloc(sizeof(short));
		}
	}
	return TCL_OK;
	error:
		if (out_sqlda != NULL) {dbi_Firebird_Free_out_sqlda(out_sqlda,i);}
		return TCL_ERROR;
}

int dbi_Firebird_Free_in_sqlda(
	XSQLDA *in_sqlda,
	int ipos)
{
	int i,dtype;
	if (in_sqlda == NULL) {return TCL_OK;}
	for(i = 0 ; i < ipos ; i++) {
		dtype = (in_sqlda->sqlvar[i].sqltype & ~1);
		switch(dtype) {
			case SQL_TEXT:
			case SQL_VARYING:
				break;
			default:
				Tcl_Free((char *)in_sqlda->sqlvar[i].sqldata);
		}
		if (in_sqlda->sqlvar[i].sqltype & 1) Tcl_Free((char *)in_sqlda->sqlvar[i].sqlind);
	}
	in_sqlda->sqld = 0;
	return TCL_OK;
}

int dbi_Firebird_Prepare_in_sqlda(
	Tcl_Interp *interp,
	XSQLDA *in_sqlda)
{
	XSQLVAR *var = NULL;
	int dtype,ipos = 0,nullable;
	for (ipos = 0, var = in_sqlda->sqlvar; ipos < in_sqlda->sqld; ipos++, var++) {
		if (var->sqltype & 1) {
			var->sqlind = (short *)Tcl_Alloc(sizeof(short));
			nullable = 1;
		} else {
			nullable = 0;
		}
		dtype = (var->sqltype & ~1);
		/* fprintf(stdout,"ipos=%d, dtype=%d\n",ipos,dtype);fflush(stdout);*/
		switch(dtype) {
			case SQL_VARYING:
				var->sqltype = SQL_TEXT | nullable;
			case SQL_TEXT:
				break;
			case SQL_SHORT:
				var->sqldata = (char *)Tcl_Alloc(sizeof(short));
				break;
			case SQL_LONG:
				var->sqldata = (char *)Tcl_Alloc(sizeof(long));
				break;
			case SQL_INT64:
				var->sqldata = (char *)Tcl_Alloc(sizeof(ISC_INT64));
				break;
			case SQL_FLOAT:
				var->sqldata = (char *)Tcl_Alloc(sizeof(long));
				break;
			case SQL_D_FLOAT:
			case SQL_DOUBLE:
				var->sqldata = (char *)Tcl_Alloc(sizeof(double));
				break;
			case SQL_TIMESTAMP:
				var->sqldata = (char *)Tcl_Alloc(sizeof(ISC_TIMESTAMP));
				break;
			case SQL_BLOB:
				{
				ISC_QUAD *blob_id;
				blob_id = (ISC_QUAD *)Tcl_Alloc(sizeof(ISC_QUAD));
				var->sqldata = (char *)blob_id;
				}
				break;
			case SQL_ARRAY:
			case SQL_QUAD:
				var->sqldata = (char *)Tcl_Alloc(sizeof(ISC_QUAD));
				break;
			case SQL_TYPE_TIME:
				var->sqldata = (char *)Tcl_Alloc(sizeof(ISC_TIME));
				break;
			case SQL_TYPE_DATE:
				var->sqldata = (char *)Tcl_Alloc(sizeof(ISC_DATE));
				break;
			default:
				Tcl_AppendResult(interp,"unsupported type", NULL);
				goto error;
		}
	}
	return TCL_OK;
	error:
		if (in_sqlda != NULL) {dbi_Firebird_Free_in_sqlda(in_sqlda,ipos);}
		return TCL_ERROR;
}

int dbi_Firebird_Fill_in_sqlda(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	XSQLDA *in_sqlda,
	int objc,
	Tcl_Obj **objv,
	Tcl_Obj *nullvalue)
{
	XSQLVAR *var = NULL;
	char *string, *nullstring;
	int error,len,dtype,size,temp,ms,ipos = 0,nulllen;
	if (nullvalue == NULL) {
		nullstring = NULL; nulllen = 0;
	} else {
		nullstring = Tcl_GetStringFromObj(nullvalue,&nulllen);
	}
	for (ipos = 0, var = in_sqlda->sqlvar; ipos < in_sqlda->sqld; ipos++, var++) {
		if (var->sqltype & 1) {
			*(var->sqlind) = 1;
			if (nullstring == NULL) {
				*(var->sqlind) = 0;
			} else {
				string = Tcl_GetStringFromObj(objv[ipos],&len);
				if ((len == nulllen)&&(strncmp(string,nullstring,len) == 0)) {
					*(var->sqlind) = -1;
					var->sqldata = NULL;
					continue;
				} else {
					*(var->sqlind) = 0;
				}
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
					Tcl_AppendResult(interp,"String argument to large",NULL);
					goto error;
				}
				var->sqllen = (short)size;
				break;
			case SQL_SHORT:
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
				error = Tcl_GetDoubleFromObj(interp,objv[ipos],&temp);
				if (error) {goto error;}
				*(float *)(var->sqldata) = (float)temp;
				}
				break;
			case SQL_D_FLOAT:
			case SQL_DOUBLE:
				{
				double temp;
				error = Tcl_GetDoubleFromObj(interp,objv[ipos],&temp);
				if (error) {goto error;}
				*(double *)(var->sqldata) = temp;
				}
				break;
			case SQL_TIMESTAMP:
				{
				struct tm times;
				int ms;
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
				ISC_QUAD *blob_id = (ISC_QUAD *)var->sqldata;
				if (dbdata->blobid) {
					error = sscanf(Tcl_GetStringFromObj(objv[ipos],NULL),"%lx:%lx", &(blob_id->QUAD_HIGH), &(blob_id->QUAD_LOW));
					if (error != 2) {
						Tcl_AppendResult(interp, "error in format of blob id",NULL);
						goto error;
					}
				} else {
					error = dbi_Firebird_Blob_Create(interp,dbdata,objv[ipos],blob_id);
					if (error) {dbi_Firebird_Error(interp,dbdata,"creating blob");goto error;}
					/*fprintf(stdout, "%08lx:%08lx\n", blob_id->QUAD_HIGH, blob_id->QUAD_LOW);fflush(stdout);*/
				}
				}
				break;
			case SQL_ARRAY:
			case SQL_QUAD:
				/*
				 * Print the blob id on blobs or arrays 
				 */
				{
				}
				break;
			case SQL_TYPE_TIME:
				{
				struct tm times;
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
	return TCL_OK;
	error:
		return TCL_ERROR;
}

int dbi_Firebird_Open(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	const static char *switches[] = {"-user","-password","-role", (char *) NULL};
	char *string=NULL;
	int len,i,dpbpos,item=0,index,error = TCL_OK;
	if (dbdata->db != NULL) {
		Tcl_AppendResult(interp,"dbi object has open connection, close first", NULL);
		goto error;
	}
	if (objc < 3) {
		Tcl_WrongNumArgs(interp,2,objv,"datasource ?options?");
		return TCL_ERROR;
	}
	dbdata->stmt = NULL;
	dbdata->trans = NULL;
	dbdata->autocommit = 1;
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
	if (error || (dbdata->db == NULL)) {
		Tcl_AppendResult(interp,"connection to database \"",
			Tcl_GetStringFromObj(dbdata->database,NULL) , "\" failed:\n", NULL);
		Tcl_DecrRefCount(dbdata->database);dbdata->database=NULL;
		dbi_Firebird_Error(interp,dbdata,"opening to database");
		goto error;
	}
	error = isc_dsql_allocate_statement(dbdata->status, &(dbdata->db), &(dbdata->stmt));
	if (error) {
		Tcl_AppendResult(interp,"Firebird error on statement allocation:\n", NULL);
		dbi_Firebird_Error(interp,dbdata,"allocating statement");
		goto error;
	}
	Tcl_InitHashTable(&(dbdata->preparedhash),TCL_STRING_KEYS);
	dbdata->out_sqlda_cache = NULL;
	return TCL_OK;
	error:
		return TCL_ERROR;
}

int dbi_Firebird_Fetch_One(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
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
				PARAMVARY *vary;
				vary = (PARAMVARY*) var->sqldata;
				textend = vary->vary_length;
				element = Tcl_NewStringObj((char *)vary->vary_string,vary->vary_length);
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
				if (dbdata->blobid) {
					char buffer[18];
					sprintf(buffer, "%lx:%lx", bid.QUAD_HIGH, bid.QUAD_LOW);
					element = Tcl_NewStringObj(buffer,-1);
				} else {
					/*fprintf(stdout, "%08lx:%08lx\n", bid.QUAD_HIGH, bid.QUAD_LOW);fflush(stdout);*/
					error = dbi_Firebird_Blob_Get(interp,dbdata,&bid,&element);
					if (error) {goto error;}
				}
				}
				break;
			case SQL_ARRAY:
			case SQL_QUAD:
				{
				ISC_QUAD bid;
				char blob_s[18];
                bid = *(ISC_QUAD *)var->sqldata;
                sprintf(blob_s, "%08lx:%08lx", bid.QUAD_HIGH, bid.QUAD_LOW);
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
		if (element != NULL) {Tcl_DecrRefCount(element);}
		return TCL_ERROR;
}

int dbi_Firebird_Fetch_Row(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	XSQLDA *out_sqlda,
	Tcl_Obj *nullvalue,
	Tcl_Obj **result)
{
	Tcl_Obj *line = NULL, *element = NULL;
	int i,error;
	line = Tcl_NewListObj(0,NULL);
	for (i = 0; i < out_sqlda->sqld; i++) {
		error = dbi_Firebird_Fetch_One(interp,dbdata,i,&element);
		if (error) {goto error;}
		if (element == NULL) {element = nullvalue;}
		error = Tcl_ListObjAppendElement(interp,line, element);
		if (error) goto error;
		element = NULL;
	}
	*result = line;
	return TCL_OK;
	error:
		if (line != NULL) {Tcl_DecrRefCount(line);}
		if (element != NULL) {Tcl_DecrRefCount(element);}
		return TCL_ERROR;
}

int dbi_Firebird_Fetch_Array(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	XSQLDA *out_sqlda,
	Tcl_Obj *nullvalue,
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
		error = dbi_Firebird_Fetch_One(interp,dbdata,i,&element);
		if (error) {goto error;}
		if (element == NULL) {element = nullvalue;}
		error = Tcl_ListObjAppendElement(interp,line, element);
		if (error) goto error;
		element = NULL;
	}
	*result = line;
	return TCL_OK;
	error:
		if (line != NULL) {Tcl_DecrRefCount(line);}
		if (element != NULL) {Tcl_DecrRefCount(element);}
		return TCL_ERROR;
}

int dbi_Firebird_Fetch_Fields(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
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
		if (line != NULL) {Tcl_DecrRefCount(line);}
		if (element != NULL) {Tcl_DecrRefCount(element);}
		return TCL_ERROR;
}

int dbi_Firebird_Fetch(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	Tcl_Obj *tuple = NULL, *field = NULL, *nullvalue = NULL;
	int fetch_option;
    const static char *subCmds[] = {
		 "data", "lines", "pos", "fields", "isnull", "array",
		(char *) NULL};
    enum ISubCmdIdx {
		Data, Lines, Pos, Fields, Isnull, Array
    };
	const static char *switches[] = {
		"-nullvalue", "-blobid",
		(char *) NULL};
    enum switchesIdx {
		Nullvalue, Blobid
    };
	int ituple = -1, ifield = -1;
	ISC_STATUS *status_vector = dbdata->status;
	isc_stmt_handle *stmt = &(dbdata->stmt);
	Tcl_Obj *line = NULL, *element = NULL;
    long fetch_stat;
	int error;
	int nfields;
	if ((dbdata->out_sqlda == NULL) || (dbdata->out_sqlda->sqld == 0)) {
		Tcl_AppendResult(interp, "no result available: invoke exec method with -usefetch option first", NULL);
		return TCL_ERROR;
	}
	nfields = dbdata->out_sqlda->sqld;
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
		dbdata->blobid = 0;
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
			case Blobid:
				dbdata->blobid = 1;
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
			Tcl_AppendResult(interp,"dbi_firebird: fetch lines not supported", NULL);
			return TCL_ERROR;
		case Fields:
			error = dbi_Firebird_Fetch_Fields(interp,dbdata,dbdata->out_sqlda,&line);
			if (error) {goto error;}
			Tcl_SetObjResult(interp, line);
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
			Tcl_AppendResult(interp,"firebird error: backwards positioning for fetch not supported",NULL);
			return TCL_ERROR;
		}
		while (dbdata->tuple < ituple) {
			fetch_stat = isc_dsql_fetch(status_vector, stmt, SQL_DIALECT_V6, dbdata->out_sqlda);
			if (fetch_stat) {dbdata->ntuples = dbdata->tuple;goto out_of_position;}
			dbdata->tuple++;		
		}
	}
	if (nullvalue == NULL) {
		nullvalue = dbdata->defnullvalue;
	}
	switch (fetch_option) {
		case Data:
			if (ifield == -1) {
				error = dbi_Firebird_Fetch_Row(interp,dbdata,dbdata->out_sqlda,nullvalue,&line);
				if (error) {return error;}
				Tcl_SetObjResult(interp, line);
				line = NULL;
			} else {
				error = dbi_Firebird_Fetch_One(interp,dbdata,ifield,&element);
				if (error) {goto error;}
				if (element == NULL) {
					Tcl_SetObjResult(interp, nullvalue);
				} else {
					Tcl_SetObjResult(interp, element);
				}
			}
			break;
		case Isnull:
			error = dbi_Firebird_Fetch_One(interp,dbdata,ifield,&element);
			if (error) {goto error;}
			if (element == NULL) {
				Tcl_SetObjResult(interp,Tcl_NewIntObj(1));
			} else {
				Tcl_SetObjResult(interp,Tcl_NewIntObj(0));
			}
			return TCL_OK;
		case Array:
			error = dbi_Firebird_Fetch_Array(interp,dbdata,dbdata->out_sqlda,nullvalue,&line);
			if (error) {return error;}
			Tcl_SetObjResult(interp, line);
			break;
	}
	return TCL_OK;
	error:
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

#ifdef never
	int dbi_Firebird_Transaction_Commit(
		Tcl_Interp *interp,
		dbi_Firebird_Data *dbdata,
		int noretain)
	{
		isc_tr_handle *trans = dbi_Firebird_trans(dbdata);
		int error,i;
	/*fprintf(stdout,"commit stmt=%X trans=%X\n",(uint)dbdata->stmt,(uint)trans);fflush(stdout);*/
		if (((dbdata->parent == NULL) && (dbdata->clonesnum == 0)) || noretain) {
			/* we can allways do a full commit safely if there are no clones,
			   or when asked to do so (explicit transactions) */
			error = isc_commit_transaction(dbdata->status, trans);
	/*fprintf(stdout,"-> committed\n");fflush(stdout);*/
		} else {
			error = isc_commit_retaining(dbdata->status, trans);
	/*fprintf(stdout,"-> commit retaining\n");fflush(stdout);*/
		}
		if (error) {
			dbi_Firebird_Error(interp,dbdata,"committing transaction");
			return TCL_ERROR;
		}
		return TCL_OK;
	}
#endif

int dbi_Firebird_Transaction_Commit(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	int noretain)
{
	isc_tr_handle *trans = dbi_Firebird_trans(dbdata);
	int error,i,clonesnum;
	dbi_Firebird_Data *parentdbdata;
/*fprintf(stdout,"commit stmt=%X trans=%X\n",(uint)dbdata->stmt,(uint)trans);fflush(stdout);*/
	/* since a full commit throws away open result sets, we have to be carefull with it */
	if (!noretain) {
		/* if explicitly asked (noretain = 1), we do a full commit, so do checks only if nretain = 0 */
		if (dbdata->blob_handle != NULL) {
		} else if ((dbdata->parent == NULL) && (dbdata->clonesnum == 0)) {
			/* parent without clones, no problem */
			noretain = 1;
		} else {
			/* retain if any of the others has a open cursor or blob */
			noretain = 1;
			if (dbdata->parent == NULL) {
				parentdbdata = dbdata;
			} else {
				/* if parent has an open cursor, use retain */
				parentdbdata = dbdata->parent;
				if (parentdbdata->cursor_open) {noretain = 0;}
				if (parentdbdata->blob_handle != NULL) {noretain = 0;}
			}
			clonesnum = parentdbdata->clonesnum;
			for (i = 0 ; i < clonesnum ; i++) {
				/* blob may never be open */
				if (parentdbdata->clones[i]->blob_handle != NULL) {noretain = 0; break;}
				/* caller may have an open cursor, that will be closed */
				if (parentdbdata->clones[i] == dbdata) continue;
				/* if any other than caller has an open cursor, use retain */
				if (parentdbdata->clones[i]->cursor_open) {noretain = 0; break;}
			}
		}
	}
	if (noretain) {
		error = isc_commit_transaction(dbdata->status, trans);
/*fprintf(stdout,"-> committed\n");fflush(stdout);*/
	} else {
		error = isc_commit_retaining(dbdata->status, trans);
/*fprintf(stdout,"-> commit retaining\n");fflush(stdout);*/
	}
	if (error) {
		dbi_Firebird_Error(interp,dbdata,"committing transaction");
		return TCL_ERROR;
	}
	return TCL_OK;
}

int dbi_Firebird_Transaction_Rollback(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata)
{
	isc_tr_handle *trans = dbi_Firebird_trans(dbdata);
	int error;
	if (*trans == NULL) {return TCL_OK;}
/*fprintf(stdout,"rollbc stmt=%X trans=%X\n",(uint)dbdata->stmt,(uint)trans);fflush(stdout);*/
	error = isc_rollback_transaction(dbdata->status, trans);
	if (error) {
		dbi_Firebird_Error(interp,dbdata,"rolling back transaction");
		return TCL_ERROR;
	}
	return TCL_OK;
}

int dbi_Firebird_Transaction_Start(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata)
{
	isc_tr_handle *trans = dbi_Firebird_trans(dbdata);
	int error;
	static char isc_tpb[] = {
		isc_tpb_version3/*,
		isc_tpb_read_committed*/
	};
	if (*trans != NULL) {return TCL_OK;}
/*fprintf(stdout,"start  stmt=%X trans=%X\n",(uint)dbdata->stmt,(uint)trans);fflush(stdout);*/
	error = isc_start_transaction(dbdata->status, trans,1,&(dbdata->db),(unsigned short) sizeof(isc_tpb),isc_tpb);
	if (error) {
		long SQLCODE;
		SQLCODE = isc_sqlcode(dbdata->status);
		if (SQLCODE == -902) {
			error = dbi_Firebird_Reconnect(interp,dbdata);
			if (error) {return TCL_ERROR;}
			error = isc_start_transaction(dbdata->status, trans,1,&(dbdata->db), (unsigned short) sizeof(isc_tpb),isc_tpb);
			if (!error) {return TCL_OK;}
		}
		dbi_Firebird_Error(interp,dbdata,"starting transaction");
		return TCL_ERROR;
	}
	return TCL_OK;
}

int dbi_Firebird_autocommit(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata)
{
	int auto_commit = dbi_Firebird_autocommit_state(dbdata);
	if (auto_commit) {
		if (dbdata->cursor_open) {dbi_Firebird_Free_Stmt(dbdata,DSQL_close);}
		if (dbi_Firebird_trans_state(dbdata)) {
			if (dbi_Firebird_Transaction_Commit(interp,dbdata,0) != TCL_OK) {return TCL_ERROR;}
		}
		if (dbi_Firebird_Transaction_Start(interp,dbdata) != TCL_OK) {return TCL_ERROR;}
	} else {
		if (dbdata->cursor_open) {
			dbi_Firebird_Free_Stmt(dbdata,DSQL_close);
		}
	}
	return TCL_OK;
}

int dbi_Firebird_Blob(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	ISC_STATUS *status_vector = dbdata->status;
	ISC_STATUS blob_stat = 0;
	ISC_QUAD blob_id;
	isc_blob_handle *blob_handle = &(dbdata->blob_handle);
	Tcl_Obj *result = NULL;
	char blob_segment[SEGM_SIZE];
	int error;
	unsigned short actual_seg_len;
	int option,sizeasked;
	long size;
    const static char *subCmds[] = {
		 "open", "skip", "get", "close",
		(char *) NULL};
    enum ISubCmdIdx {
		Open, Skip, Get, Close
    };
	/*
	 * decode options
	 * --------------
	 */
	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "option ?...?");
		return TCL_ERROR;
	}
	error = Tcl_GetIndexFromObj(interp, objv[2], subCmds, "option", 0, (int *) &option);
	if (error) {return error;}
	switch (option) {
	case Open:
		if (objc != 4) {
			Tcl_WrongNumArgs(interp, 3, objv, "blobid");
			return TCL_ERROR;
		}
		error = sscanf(Tcl_GetStringFromObj(objv[3],NULL),"%lx:%lx", &(blob_id.QUAD_HIGH), &(blob_id.QUAD_LOW));
		if (error != 2) {
			Tcl_AppendResult(interp, "error in format of blob id",NULL);
			return TCL_ERROR;
		}
		if (*blob_handle != NULL) {
			if (isc_close_blob(status_vector, blob_handle)) {
				Tcl_AppendResult(interp, "error closing blob ",Tcl_GetStringFromObj(objv[2],NULL),": ",NULL);dbi_Firebird_Error(interp,dbdata,"");
				return TCL_ERROR;
			}
			*blob_handle = NULL;
		}
		error = dbi_Firebird_autocommit(interp,dbdata);
		if (error) {goto error;}
		error = isc_open_blob2(status_vector, &(dbdata->db),  dbi_Firebird_trans(dbdata), blob_handle, &blob_id, 0, NULL);
		if (error) {
			Tcl_AppendResult(interp, "error opening blob ",Tcl_GetStringFromObj(objv[2],NULL),": ",NULL);dbi_Firebird_Error(interp,dbdata,"");return TCL_ERROR;
		}
		return TCL_OK;
	case Skip:
		if (objc != 4) {
			Tcl_WrongNumArgs(interp, 3, objv, "size");
			return TCL_ERROR;
		}
		error = Tcl_GetLongFromObj(interp,objv[3],&size);
		if (error) {return error;}
		actual_seg_len = SEGM_SIZE;
		while (actual_seg_len) {
			if (size < SEGM_SIZE) {
				actual_seg_len = (int)size;
				sizeasked = (int)size;
			} else {
				sizeasked = SEGM_SIZE;
			}
			blob_stat = isc_get_segment(status_vector, blob_handle,
				&actual_seg_len,sizeasked,blob_segment);
			size -= actual_seg_len;
			if (size == 0) break;
			if (blob_stat == isc_segstr_eof) break;
			if (blob_stat) {dbi_Firebird_Error(interp,dbdata,"skipping blob");goto error;}
		}
		return TCL_OK;
	case Get:
		if ((objc != 3)&&(objc != 4)) {
			Tcl_WrongNumArgs(interp, 3, objv, "?size?");
			return TCL_ERROR;
		}
		if (objc == 4) {
			error = Tcl_GetLongFromObj(interp,objv[3],&size);
			if (error) {return error;}
		} else {
			size = -1;
		}
		result = Tcl_NewStringObj("",0);
		actual_seg_len = SEGM_SIZE;
		while (actual_seg_len) {
			if ((size > 0) && (size < SEGM_SIZE)) {
				actual_seg_len = (int)size;
				sizeasked = (int)size;
			} else {
				sizeasked = SEGM_SIZE;
			}
			blob_stat = isc_get_segment(status_vector, blob_handle,
				&actual_seg_len,sizeasked,blob_segment);
			Tcl_AppendToObj(result,blob_segment,actual_seg_len);
			size -= actual_seg_len;
			if (size == 0) break;
			if (blob_stat == isc_segstr_eof) break;
			if (blob_stat) {dbi_Firebird_Error(interp,dbdata,"getting blob");goto error;}
		}
		Tcl_SetObjResult(interp,result);
		return TCL_OK;
	case Close:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 3, objv, "");
			return TCL_ERROR;
		}
		if (*blob_handle == NULL) {
			return TCL_OK;
		} else {
			if (isc_close_blob(status_vector, blob_handle)) {
				Tcl_AppendResult(interp, "error closing blob ",Tcl_GetStringFromObj(objv[2],NULL),": ",NULL);dbi_Firebird_Error(interp,dbdata,"");
				return TCL_ERROR;
			}
			*blob_handle = NULL;
		}
		return TCL_OK;
	}
	return TCL_OK;
	error:
		if (result != NULL) {Tcl_DecrRefCount(result);}
		return TCL_ERROR;
}

int dbi_Firebird_NewBlob(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	ISC_STATUS *status_vector = dbdata->status;
	ISC_QUAD *blob_id = &(dbdata->newblob_id);
	isc_blob_handle *blob_handle = &(dbdata->newblob_handle);
	char *buffer = dbdata->newblob_buffer;
	char idbuffer[18];
	char *string;
	int error;
	int option,stringlen;
    const static char *subCmds[] = {
		 "create", "put", "close",
		(char *) NULL};
    enum ISubCmdIdx {
		Create, Put, Close
    };
	/*
	 * decode options
	 * --------------
	 */
	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 2, objv, "option ?...?");
		return TCL_ERROR;
	}
	error = Tcl_GetIndexFromObj(interp, objv[2], subCmds, "option", 0, (int *) &option);
	if (error) {return error;}
	if (dbi_Firebird_autocommit_state(dbdata)) {
		Tcl_AppendResult(interp,"newblob is only useable within an explicit transaction", NULL);
		return TCL_ERROR;
	}
	switch (option) {
	case Create:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 3, objv, "");
			return TCL_ERROR;
		}
		if (*blob_handle != NULL) {
			Tcl_AppendResult(interp,"blob still open", NULL);
			goto error;
		}
		error = dbi_Firebird_autocommit(interp,dbdata);
		if (error) {goto error;}
		error = isc_create_blob2(status_vector, &(dbdata->db), dbi_Firebird_trans(dbdata), blob_handle, blob_id, 0, NULL);
		if (error) {goto error;}
		dbdata->newblob_buffer_free = SEGM_SIZE;
		return TCL_OK;
	case Put:
		{
		int bufferfree = dbdata->newblob_buffer_free;
		if (objc != 4) {
			Tcl_WrongNumArgs(interp, 3, objv, "data");
			return TCL_ERROR;
		}
		string = Tcl_GetStringFromObj(objv[3],&stringlen);
		while (stringlen > 0) {
			if (stringlen > bufferfree) {
				memcpy(buffer+SEGM_SIZE-bufferfree,string,bufferfree);
				string += bufferfree;
				stringlen -= bufferfree;
				bufferfree = 0;
			} else {
				memcpy(buffer+SEGM_SIZE-bufferfree,string,stringlen);
				bufferfree -= stringlen;
				stringlen = 0;
			}
			if (!bufferfree) {
				error = isc_put_segment(status_vector, blob_handle, SEGM_SIZE-bufferfree, buffer);
				if (error) {goto error;}
				bufferfree = SEGM_SIZE;
			}
		}
		dbdata->newblob_buffer_free = bufferfree;
		return TCL_OK;
		}
	case Close:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 3, objv, "");
			return TCL_ERROR;
		}
		if (*blob_handle == NULL) {
			Tcl_AppendResult(interp,"no blob created", NULL);
			goto error;
		}
		if (dbdata->newblob_buffer_free != SEGM_SIZE) {
			error = isc_put_segment(status_vector, blob_handle, SEGM_SIZE-dbdata->newblob_buffer_free, buffer);
			if (error) {goto error;}
		}
		error = isc_close_blob(status_vector, blob_handle);
		if (error) {goto error;}
		*blob_handle = NULL;
		sprintf(idbuffer, "%lx:%lx", blob_id->QUAD_HIGH, blob_id->QUAD_LOW);
		Tcl_AppendResult(interp,idbuffer,NULL);
		return TCL_OK;
	}
	return TCL_OK;
	error:
		return TCL_ERROR;
}

int dbi_Firebird_ToResult(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	XSQLDA *out_sqlda,
	Tcl_Obj *nullvalue)
{
	ISC_STATUS *status_vector = dbdata->status;
	isc_stmt_handle *stmt = &(dbdata->stmt);
	Tcl_Obj *result = NULL, *line = NULL;
    long fetch_stat;
	int error;
	if (nullvalue == NULL) {nullvalue = dbdata->defnullvalue;}
	result = Tcl_NewListObj(0,NULL);
	while (1) {
		fetch_stat = isc_dsql_fetch(status_vector, stmt, SQL_DIALECT_V6, out_sqlda);
		if (fetch_stat) break;
		error = dbi_Firebird_Fetch_Row(interp,dbdata,out_sqlda,nullvalue,&line);
		if (error) {goto error;}
		error = Tcl_ListObjAppendElement(interp,result, line);
		if (error) goto error;
		line = NULL;
	}
	Tcl_SetObjResult(interp, result);
	return TCL_OK;
	error:
		if (result != NULL) {Tcl_DecrRefCount(result);}
		if (line != NULL) {Tcl_DecrRefCount(line);}
		return TCL_ERROR;
}

int dbi_Firebird_ToResult_flat(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	XSQLDA *out_sqlda,
	Tcl_Obj *nullvalue)
{
	ISC_STATUS *status_vector = dbdata->status;
	isc_stmt_handle *stmt = &(dbdata->stmt);
	Tcl_Obj *result = NULL, *element = NULL;
    long fetch_stat;
	int error,i;
	if (nullvalue == NULL) {nullvalue = dbdata->defnullvalue;}
	result = Tcl_NewListObj(0,NULL);
	while (1) {
		fetch_stat = isc_dsql_fetch(status_vector, stmt, SQL_DIALECT_V6, out_sqlda);
		if (fetch_stat) break;
		for (i = 0; i < out_sqlda->sqld; i++) {
			error = dbi_Firebird_Fetch_One(interp,dbdata,i,&element);
			if (error) {goto error;}
			if (element == NULL) {element = nullvalue;}
			error = Tcl_ListObjAppendElement(interp,result,element);
			if (error) goto error;
			element = NULL;
		}
	}
	Tcl_SetObjResult(interp, result);
	return TCL_OK;
	error:
		if (result != NULL) {Tcl_DecrRefCount(result);}
		if (element != NULL) {Tcl_DecrRefCount(element);}
		return TCL_ERROR;
}

int dbi_Firebird_Line_empty(
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

int dbi_Firebird_Find_Prev(
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

int dbi_Firebird_Cache_stmt(
	dbi_Firebird_Data *dbdata)
{
	dbdata->out_sqlda_cache = dbdata->out_sqlda;
	dbdata->in_sqlda_cache = dbdata->in_sqlda;
	return TCL_OK;
}

int dbi_Firebird_Restore_stmt(
	dbi_Firebird_Data *dbdata)
{
	if (dbdata->out_sqlda_cache != NULL) {
		dbdata->out_sqlda = dbdata->out_sqlda_cache;
		dbdata->out_sqlda_cache = NULL;
	}
	if (dbdata->in_sqlda_cache != NULL) {
		dbdata->in_sqlda = dbdata->in_sqlda_cache;
		dbdata->in_sqlda_cache = NULL;
	}
	return TCL_OK;
}

int dbi_Firebird_Prepare_Cache(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	int cmdlen,
	char *cmdstring,
	int *newPtr
)
{
	Tcl_HashEntry *entry;
	dbi_Firebird_Prepared *prepared;
	int new = 0;
	/* save current stmt if needed */
	if (dbdata->out_sqlda_cache == NULL) {
		dbi_Firebird_Cache_stmt(dbdata);
	}
	/* find cached entry, or create new */
	entry = Tcl_CreateHashEntry(&(dbdata->preparedhash), cmdstring, &new);
	/* create new stmt if needed */
	if (new) {
		/* create new stmt */
		Dbi_firebird_initsqlda(&(dbdata->out_sqlda));
		Dbi_firebird_initsqlda(&(dbdata->in_sqlda));
		/* cache information for current query */
		prepared = (dbi_Firebird_Prepared *)Tcl_Alloc(sizeof(dbi_Firebird_Prepared));
		prepared->out_sqlda = dbdata->out_sqlda;
		prepared->in_sqlda = dbdata->in_sqlda;
		Tcl_SetHashValue(entry,(ClientData)prepared);
	} else {
		/* cache found: use prepared statement, cache current if needed */
		prepared = (dbi_Firebird_Prepared *)Tcl_GetHashValue(entry);
		dbdata->out_sqlda = prepared->out_sqlda;
		dbdata->in_sqlda = prepared->in_sqlda;
	}
	*newPtr = new;
	return TCL_OK;
}

int dbi_Firebird_Prepare_statement(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	int cmdlen,
	char *cmdstring)
{
	ISC_STATUS *status_vector = dbdata->status;
	int error,n = 0;
	error = isc_dsql_prepare(status_vector, dbi_Firebird_trans(dbdata), &(dbdata->stmt), cmdlen, cmdstring, SQL_DIALECT_V6, dbdata->out_sqlda);
	if (error) {
		dbi_Firebird_Error(interp,dbdata,"preparing statement");
		goto error;
	}
	/* ----- Get information about input of cmd ----- */
	error = isc_dsql_describe_bind(status_vector, &(dbdata->stmt), SQL_DIALECT_V6, dbdata->in_sqlda);
	if (error) {goto error;}
	if (dbdata->in_sqlda->sqld > dbdata->in_sqlda->sqln) {
		n = dbdata->in_sqlda->sqld;
		Tcl_Free((char *)dbdata->in_sqlda);
		dbdata->in_sqlda = (XSQLDA *)Tcl_Alloc(XSQLDA_LENGTH(n));
		dbdata->in_sqlda->sqln = n;
		dbdata->in_sqlda->sqld = 0;
		dbdata->in_sqlda->version = SQLDA_VERSION1;
		isc_dsql_describe_bind(status_vector, &(dbdata->stmt), SQL_DIALECT_V6, dbdata->in_sqlda);
	}
	error = dbi_Firebird_Prepare_in_sqlda(interp,dbdata->in_sqlda);
	if (error) {goto error;}
	/* ----- expand out_sqlda as necessary ----- */
	if ((dbdata->out_sqlda != NULL) && (dbdata->out_sqlda->sqld > 0)) {
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
			error = isc_dsql_describe(status_vector, &(dbdata->stmt), SQL_DIALECT_V6, dbdata->out_sqlda);
			if (error) {
				Tcl_AppendResult(interp,"database error in describe\n", NULL);
				goto error;
			}
		}
		error = dbi_Firebird_Prepare_out_sqlda(interp,dbdata->out_sqlda);
		if (error) {
			Tcl_AppendResult(interp,"database error in prepare\n", NULL);
			goto error;
		}
	}
	return TCL_OK;
	error:
		if (dbdata->out_sqlda != NULL) {dbi_Firebird_Free_out_sqlda(dbdata->out_sqlda,dbdata->out_sqlda->sqld);}
		if (dbdata->in_sqlda != NULL) {dbi_Firebird_Free_in_sqlda(dbdata->in_sqlda,dbdata->in_sqlda->sqld);}
		{
		Tcl_Obj *temp;
		temp = Tcl_NewStringObj(cmdstring,cmdlen);
		Tcl_AppendResult(interp," while preparing command: \"",	Tcl_GetStringFromObj(temp,NULL), "\"", NULL);
		Tcl_DecrRefCount(temp);
		}
		return TCL_ERROR;		
}

int dbi_Firebird_Process_statement(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	int cmdlen,
	char *cmdstring)
{
	ISC_STATUS *status_vector = dbdata->status;
	static char stmt_info[] = { isc_info_sql_stmt_type };
	char info_buffer[20];
	short l;
	int error;
	/* Get information about output of cmd */
	if ((dbdata->out_sqlda == NULL) || (dbdata->out_sqlda->sqld == 0)) {
		/* What is the statement type of this statement? 
		**
		** stmt_info is a 1 byte info request.  info_buffer is a buffer
		** large enough to hold the returned info packet
		** The info_buffer returned contains a isc_info_sql_stmt_type in the first byte, 
		** two bytes of length, and a statement_type token.
		*/
		long statement_type;
		error = isc_dsql_sql_info(status_vector, &(dbdata->stmt), sizeof(stmt_info), stmt_info, sizeof(info_buffer), info_buffer);
		if (!error)	{
			l = (short) isc_vax_integer((char ISC_FAR *)info_buffer + 1, 2);
			statement_type = isc_vax_integer((char ISC_FAR *)info_buffer + 3, l);
		} else {
			statement_type = 0;
		}
		switch (statement_type) {
			case isc_info_sql_stmt_start_trans:
				if (dbi_Firebird_trans_state(dbdata)) {
					if (dbdata->cursor_open != 0) {dbi_Firebird_Free_Stmt(dbdata,DSQL_close);}
					error = dbi_Firebird_Transaction_Commit(interp,dbdata,1);
				}
				if (error) {goto error;}
				error = isc_dsql_execute_immediate(status_vector, &(dbdata->db), 
					dbi_Firebird_trans(dbdata), 0, cmdstring, SQL_DIALECT_V6, dbdata->in_sqlda);
				if (error) {dbi_Firebird_Error(interp,dbdata,"executing start statement");goto error;}
				dbi_Firebird_autocommit_set(dbdata,0);
				goto clean;
				break;
			case isc_info_sql_stmt_commit:
				error = isc_dsql_execute_immediate(status_vector, &(dbdata->db), 
					dbi_Firebird_trans(dbdata), 0, cmdstring, SQL_DIALECT_V6, dbdata->in_sqlda);
				dbi_Firebird_autocommit_set(dbdata,1);
				if (error) {dbi_Firebird_Error(interp,dbdata,"executing commit statement");goto error;}
				goto clean;
				break;
			case isc_info_sql_stmt_rollback:
				error = isc_dsql_execute_immediate(status_vector, &(dbdata->db), 
					dbi_Firebird_trans(dbdata), 0, cmdstring, SQL_DIALECT_V6, dbdata->in_sqlda);
				dbi_Firebird_autocommit_set(dbdata,1);
				if (error) {dbi_Firebird_Error(interp,dbdata,"executing rollback statement");goto error;}
				goto clean;
				break;
		}
	}
	/* Execute prepared stmt */
/*fprintf(stdout,"exec   stmt=%X trans=%X, in = %X, out = %X\n",(uint)dbdata->stmt,(uint)dbi_Firebird_trans(dbdata),(uint)dbdata->in_sqlda,(uint)dbdata->out_sqlda);fflush(stdout);*/
	error = isc_dsql_execute(status_vector, dbi_Firebird_trans(dbdata), &(dbdata->stmt), SQL_DIALECT_V6, dbdata->in_sqlda);
	if (error) {
		dbi_Firebird_Error(interp,dbdata,"executing statement");
		goto error;
	}
	if ((dbdata->out_sqlda != NULL) && (dbdata->out_sqlda->sqld != 0)) {
/*		Tcl_Obj *dbcmd = Tcl_NewObj();
		Tcl_GetCommandFullName(interp, dbdata->token, dbcmd);
		error = isc_dsql_set_cursor_name(status_vector, &(dbdata->stmt), Tcl_GetStringFromObj(dbcmd,NULL), (short)NULL);
		Tcl_DecrRefCount(dbcmd);
		if (error) {
			dbi_Firebird_Error(interp,dbdata,"setting cursor");
			goto error;
		}
*/
		dbdata->cursor_open = 1;
	}
	clean:
	return TCL_OK;
	error:
		return TCL_ERROR;		
}

int dbi_Firebird_SplitSQL(
	Tcl_Interp *interp,
	char *cmdstring,
	int cmdlen,
	char ***linesPtr,
	int **sizesPtr)
{
	char **lines;
	char *nextline, *find;
	int *sizes;
	int i,blevel,level,start,prevline,num,len;
	nextline = strchr(cmdstring,';');
	if (nextline == NULL) {
		*linesPtr = NULL;
		*sizesPtr = NULL;
		return TCL_OK;
	}
	lines = NULL;
	sizes = NULL;
	i = 0;
	num = 0;
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
			if (dbi_Firebird_Find_Prev(cmdstring+prevline,i-prevline,"begin")) {
				blevel++;
			} else if (dbi_Firebird_Find_Prev(cmdstring+prevline,i-prevline,"BEGIN")) {
				blevel++;
			} else if (dbi_Firebird_Find_Prev(cmdstring+prevline,i-prevline,"end")) {
				blevel--;
			} else if (dbi_Firebird_Find_Prev(cmdstring+prevline,i-prevline,"END")) {
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
			if (dbi_Firebird_Find_Prev(cmdstring+prevline,i-prevline,"end")) {
				blevel--;
			} else if (dbi_Firebird_Find_Prev(cmdstring+prevline,i-prevline,"END")) {
				blevel--;
			}				find = strstr(cmdstring+prevline,"end");
			if (((level != 0)||(blevel != 0))&&(i != cmdlen)) {
				i++;
				continue;
			}
			if (i == cmdlen) break;
			/* fprintf(stdout,"** %*.*s\n",i-start,i-start,cmdstring+start);fflush(stdout); */
			lines = (char **)Tcl_Realloc((char *)lines,(num+2)*sizeof(char *));
			sizes = (int *)Tcl_Realloc((char *)sizes,(num+2)*sizeof(int));
			lines[num] = cmdstring+start;
			sizes[num] = i-start;
			num++;
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
	if (num == 0) {
		return TCL_OK;
	}
	lines[num] = NULL;
	sizes[num] = -1;
	*linesPtr = lines;
	*sizesPtr = sizes;
	return TCL_OK;
}

int dbi_Firebird_Exec(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	Tcl_Obj *cmd,
	int flags,
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	ISC_STATUS *status_vector = dbdata->status;
	char *cmdstring = NULL, **lines = NULL;
	int error,cmdlen,num,new;
	int flat, usefetch, cache,*sizes = NULL;
	flat = flags & EXEC_FLAT;
	usefetch = flags & EXEC_USEFETCH;
	cache = flags & EXEC_CACHE;
	dbdata->blobid = flags & EXEC_BLOBID;
	if (dbdata->db == NULL) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		return TCL_ERROR;
	}
	cmdstring = Tcl_GetStringFromObj(cmd,&cmdlen);
/*
fprintf(stdout," --------- %s ----------\n",cmdstring);fflush(stdout);
fprintf(stdout,"autocommit=%d transaction_state=%d\n",dbi_Firebird_autocommit_state(dbdata),dbi_Firebird_trans_state(dbdata));fflush(stdout);
*/
	if (cmdlen == 0) {return TCL_OK;}
	if (dbdata->cursor_open) {
		dbi_Firebird_Free_Stmt(dbdata,DSQL_close);
	}
	error = dbi_Firebird_autocommit(interp,dbdata);
	if (error) {goto error;}
	error = dbi_Firebird_SplitSQL(interp,cmdstring,cmdlen,&lines,&sizes);
	if (lines == NULL) {
		if (cache) {
			error = dbi_Firebird_Prepare_Cache(interp,dbdata,cmdlen,cmdstring,&new);
			if (error) {goto error;}
		} else	if (dbdata->out_sqlda_cache != NULL) {
			dbi_Firebird_Restore_stmt(dbdata);
		}
		if (!cache || new) {
			error = dbi_Firebird_Prepare_statement(interp,dbdata,cmdlen,cmdstring);
			if (error) {/*dbi_Firebird_autocommit_set(dbdata,1);*/goto error;}
		} else {
			error = isc_dsql_prepare(status_vector, dbi_Firebird_trans(dbdata), &(dbdata->stmt), cmdlen, cmdstring, SQL_DIALECT_V6, dbdata->out_sqlda);
			if (error) {
				dbi_Firebird_Error(interp,dbdata,"preparing statement");
				goto error;
			}
		}
		if (objc != dbdata->in_sqlda->sqld) {
			Tcl_AppendResult(interp,"wrong number of arguments given to exec",NULL);
			goto error;
		}
		error = dbi_Firebird_Fill_in_sqlda(interp,dbdata,dbdata->in_sqlda,objc,objv,nullvalue);
		if (error) {goto error;}
		error = dbi_Firebird_Process_statement(interp,dbdata,cmdlen,cmdstring);
		if (error) {/*dbi_Firebird_autocommit_set(dbdata,1);*/goto error;}
		if (dbdata->out_sqlda_cache == NULL) {
			if (dbdata->in_sqlda != NULL) {dbi_Firebird_Free_in_sqlda(dbdata->in_sqlda,dbdata->in_sqlda->sqld);}
		}
	} else {
		if (objc != 0) {
			Tcl_AppendResult(interp,"multiple commands cannot take arguments\n", NULL);
			goto error;
		}
		if (cache) {
			Tcl_AppendResult(interp,"multiple commands cannot be cached\n", NULL);
			goto error;
		}
		if (dbdata->out_sqlda_cache != NULL) {
			dbi_Firebird_Restore_stmt(dbdata);
		}
		num = 0;
		while (lines[num] != NULL) {
			error = dbi_Firebird_Prepare_statement(interp,dbdata,sizes[num],lines[num]);
			if (error) {/*dbi_Firebird_autocommit_set(dbdata,1);*/goto error;}
			if (objc != dbdata->in_sqlda->sqld) {
				Tcl_AppendResult(interp,"wrong number of arguments given to exec",NULL);
				goto error;
			}
			error = dbi_Firebird_Fill_in_sqlda(interp,dbdata,dbdata->in_sqlda,objc,objv,nullvalue);
			if (error) {goto error;}
			error = dbi_Firebird_Process_statement(interp,dbdata,sizes[num],lines[num]);
			if (error) {/*dbi_Firebird_autocommit_set(dbdata,1);*/ goto error;}
			if (dbdata->out_sqlda_cache == NULL) {
				if (dbdata->in_sqlda != NULL) {dbi_Firebird_Free_in_sqlda(dbdata->in_sqlda,dbdata->in_sqlda->sqld);}
			}
			num++;
		}
		Tcl_Free((char *)lines);lines = NULL;
		Tcl_Free((char *)sizes);sizes = NULL;
	}
	if ((dbdata->out_sqlda != NULL) && (dbdata->out_sqlda->sqld != 0)) {
		if (!usefetch) {
			if (flat) {
				error = dbi_Firebird_ToResult_flat(interp,dbdata,dbdata->out_sqlda,nullvalue);
			} else {
				error = dbi_Firebird_ToResult(interp,dbdata,dbdata->out_sqlda,nullvalue);
			}
			if (error) {goto error;}
			if (dbdata->cursor_open) {dbi_Firebird_Free_Stmt(dbdata,DSQL_close);}
			if ((dbi_Firebird_autocommit_state(dbdata))&&(dbi_Firebird_trans_state(dbdata))) {
				if (dbi_Firebird_Transaction_Commit(interp,dbdata,0) != TCL_OK) {
					return TCL_ERROR;
				}
			}
		} else {
			dbdata->tuple = -1;
			dbdata->ntuples = -1;
		}
	} else {
		if ((dbi_Firebird_autocommit_state(dbdata))&&(dbi_Firebird_trans_state(dbdata))) {
			if (dbi_Firebird_Transaction_Commit(interp,dbdata,0) != TCL_OK) {
				return TCL_ERROR;
			}
		}
	}
	return TCL_OK;
	error:
		{
		Tcl_Obj *temp;
		temp = Tcl_NewStringObj(cmdstring,cmdlen);
		Tcl_AppendResult(interp," while executing command: \"",	Tcl_GetStringFromObj(temp,NULL), "\"", NULL);
		Tcl_DecrRefCount(temp);
		}
		if (lines != NULL) {Tcl_Free((char *)lines);}
		if (sizes != NULL) {Tcl_Free((char *)sizes);}
		if ((dbi_Firebird_autocommit_state(dbdata))&&(dbi_Firebird_trans_state(dbdata))) {
			if (dbdata->cursor_open) {dbi_Firebird_Free_Stmt(dbdata,DSQL_close);}
			dbi_Firebird_Transaction_Rollback(interp,dbdata);
		}
		return TCL_ERROR;
}

int dbi_Firebird_Supports(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	Tcl_Obj *keyword)
{
	const static char *keywords[] = {
		"lines","backfetch","serials","sharedserials","blobparams","blobids",
		"transactions","sharedtransactions","foreignkeys","checks","views",
		"columnperm","roles","domains","permissions",
		(char *) NULL};
	static int supports[] = {
		0,0,1,1,1,1,
		1,1,1,1,1,
		1,1,1,1};
	enum keywordsIdx {
		Lines,Backfetch,Serials,Sharedserials,Blobparams, Blobids,
		Transactions,Sharedtransactions,Foreignkeys,Checks,Views,
		Columnperm, Roles, Domains, Permissions
	};
	int error,index;
	if (keyword == NULL) {
		char **keyword = (char **)keywords;
		int *value = supports;
		int index = 0;
		while (1) {
			if (*keyword == NULL) break;
			Tcl_AppendElement(interp,*keyword);
			if (value) {
				Tcl_AppendElement(interp,"1");
			} else {
				Tcl_AppendElement(interp,"0");
			}
			keyword++;
			value++;
			index++;
		}
	} else {
		error = Tcl_GetIndexFromObj(interp, keyword, keywords, "", 0, (int *) &index);
		if (error == TCL_OK) {
			switch(index) {
				default:
					Tcl_SetObjResult(interp,Tcl_NewIntObj(supports[index]));
			}
		} else {
			Tcl_AppendResult(interp,"unknown supports option: \"",	Tcl_GetStringFromObj(keyword,NULL), "\"", NULL);
			return TCL_ERROR;
		}
	}
	return TCL_OK;
}

int dbi_Firebird_Info(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	int objc,
	Tcl_Obj **objv)
{
	Tcl_Obj *cmd,*dbcmd;
	int error,i;
	dbcmd = Tcl_NewObj();
	Tcl_GetCommandFullName(interp, dbdata->token, dbcmd);
	cmd = Tcl_NewStringObj("::dbi::firebird::info",-1);
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

int dbi_Firebird_Tables(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata)
{
	int error;
	Tcl_Obj *cmd = Tcl_NewStringObj("select rdb$relation_name from rdb$relations where RDB$SYSTEM_FLAG = 0",-1);
	error = dbi_Firebird_Exec(interp,dbdata,cmd,EXEC_FLAT,NULL,0,NULL);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Firebird_Cmd(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
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

int dbi_Firebird_Serial(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	Tcl_Obj *subcmd,
	int objc,
	Tcl_Obj **objv)
{
	int error,index;
    const static char *subCmds[] = {
		"add", "delete", "set", "next", "share",
		(char *) NULL};
    enum ISubCmdIdx {
		Add, Delete, Set, Next, Share
    };
	error = Tcl_GetIndexFromObj(interp, subcmd, subCmds, "suboption", 0, (int *) &index);
	if (error != TCL_OK) {
		return error;
	}
    switch (index) {
	    case Add:
			error = dbi_Firebird_TclEval(interp,dbdata,"::dbi::firebird::serial_add",objc,objv);	
			break;
		case Delete:
			error = dbi_Firebird_TclEval(interp,dbdata,"::dbi::firebird::serial_delete",objc,objv);	
			break;
		case Set:
			error = dbi_Firebird_TclEval(interp,dbdata,"::dbi::firebird::serial_set",objc,objv);	
			break;
		case Next:
			error = dbi_Firebird_TclEval(interp,dbdata,"::dbi::firebird::serial_next",objc,objv);	
			break;
		case Share:
			error = dbi_Firebird_TclEval(interp,dbdata,"::dbi::firebird::serial_share",objc,objv);	
			break;
	}
	return error;
}

int dbi_Firebird_Createdb(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	const static char *switches[] = {"-user","-password","-pagesize","-defaultcharset","-extra", (char *) NULL};
    enum switchesIdx {User, Password, Pagesize, Defaultcharset, Extra};
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
			case User:
				if (i == (objc)) {
					Tcl_AppendResult(interp,"\"-user\" option must be followed by the username",NULL);
					return TCL_ERROR;
				}
				Tcl_AppendToObj(cmd," user '",-1);
				Tcl_AppendObjToObj(cmd,objv[i+1]);
				Tcl_AppendToObj(cmd,"'",1);
				i++;
				break;
			case Password:
				if (i == (objc-1)) {
					Tcl_AppendResult(interp,"\"-password\" option must be followed by the password of the user",NULL);
					return TCL_ERROR;
				}
				Tcl_AppendToObj(cmd," password '",-1);
				Tcl_AppendObjToObj(cmd,objv[i+1]);
				Tcl_AppendToObj(cmd,"'",1);
				i++;
				break;
			case Pagesize:
				if (i == (objc-1)) {
					Tcl_AppendResult(interp,"\"-pagesize\" option must be followed by the pagesize",NULL);
					return TCL_ERROR;
				}
				Tcl_AppendToObj(cmd," page_size ",-1);
				Tcl_AppendObjToObj(cmd,objv[i+1]);
				i++;
				break;
			case Defaultcharset:
				if (i == (objc-1)) {
					Tcl_AppendResult(interp,"\"-defaultcharset\" option must be followed by the default character set",NULL);
					return TCL_ERROR;
				}
				Tcl_AppendToObj(cmd," default character set ",-1);
				Tcl_AppendObjToObj(cmd,objv[i+1]);
				i++;
				break;
			case Extra:
				if (i == (objc-1)) {
					Tcl_AppendResult(interp,"\"-extra\" option must be followed by the extra parameters",NULL);
					return TCL_ERROR;
				}
				Tcl_AppendToObj(cmd," ",-1);
				Tcl_AppendObjToObj(cmd,objv[i+1]);
				i++;
				break;
		}
	}
	string = Tcl_GetStringFromObj(cmd,&len);
	error = isc_dsql_execute_immediate(status_vector, &(dbdata->db), dbi_Firebird_trans(dbdata),
		0,string, SQL_DIALECT_V6, NULL);
	if (error) {
		Tcl_AppendResult(interp,"creation of database \"",
			Tcl_GetStringFromObj(objv[2],NULL) , "\" failed:\n", NULL);
		dbi_Firebird_Error(interp,dbdata,"");
		goto error;
	}
	isc_detach_database(status_vector, &(dbdata->db));
	Tcl_DecrRefCount(cmd);
	return TCL_OK;
	error:
		if (cmd != NULL) {Tcl_DecrRefCount(cmd);}
		return TCL_ERROR;
}

int dbi_Firebird_Close(
	dbi_Firebird_Data *dbdata)
{
	ISC_STATUS *status_vector = dbdata->status;
	dbi_Firebird_Prepared *prepared;
	Tcl_HashSearch search;
	Tcl_HashEntry *entry;
	if (dbdata->out_sqlda != NULL) {
		dbi_Firebird_Free_out_sqlda(dbdata->out_sqlda,dbdata->out_sqlda->sqld);
	}
	if (dbdata->cursor_open) {dbi_Firebird_Free_Stmt(dbdata,DSQL_close);}
	if (dbdata->out_sqlda_cache != NULL) {
		dbi_Firebird_Restore_stmt(dbdata);
	}
	/* delete statement */
	if (dbdata->stmt != NULL) {
		dbi_Firebird_Free_Stmt(dbdata,DSQL_drop);
		dbdata->stmt = NULL;
	}
	/* delete all cached statements */
	entry = Tcl_FirstHashEntry(&(dbdata->preparedhash),&search);
	while (entry != NULL) {
		prepared = (dbi_Firebird_Prepared *)Tcl_GetHashValue(entry);
		if (prepared->out_sqlda != NULL) {
			dbi_Firebird_Free_out_sqlda(prepared->out_sqlda,prepared->out_sqlda->sqld);
			Tcl_Free((char *)prepared->out_sqlda);
		}
		if (prepared->in_sqlda != NULL) {
			dbi_Firebird_Free_in_sqlda(prepared->in_sqlda,prepared->in_sqlda->sqld);
			Tcl_Free((char *)prepared->in_sqlda);
		}
		entry = Tcl_NextHashEntry(&search);
	}
	Tcl_DeleteHashTable(&(dbdata->preparedhash));
	/* delete all clones */
	while (dbdata->clonesnum) {
		Tcl_DeleteCommandFromToken(dbdata->interp,dbdata->clones[0]->token);
	}
	if (dbdata->clones != NULL) {
		Tcl_Free((char *)dbdata->clones);
		dbdata->clones = NULL;
	}
	if (dbdata->parent == NULL) {
		/* this is not a clone, so really close */
		if (dbdata->trans != NULL) {
			isc_rollback_transaction(status_vector, &(dbdata->trans));
			dbdata->trans = NULL;
		}
		if (dbdata->database != NULL) {
			Tcl_DecrRefCount(dbdata->database);dbdata->database = NULL;
		}
		if (dbdata->db != NULL) {
			if (isc_detach_database(dbdata->status, &(dbdata->db))) {
				dbi_Firebird_Error(dbdata->interp,dbdata,"closing connection");
				dbdata->db = NULL;
				return TCL_ERROR;
			}
			dbdata->db = NULL;
		}
		if (dbdata->dpb != NULL) {
			Tcl_Free(dbdata->dpb);
			dbdata->dpblen = 0;
			dbdata->dpb = NULL;
		}
	} else {
		/* this is a clone, so remove the clone from its parent */
		int i;
		dbi_Firebird_Data *parent = dbdata->parent;
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

int dbi_Firebird_Dropdb(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata)
{
	int error;
	if (dbdata->parent != NULL) {
		Tcl_AppendResult(interp,"cannot drop database from a clone", NULL);
		return TCL_ERROR;
	}
	if (dbdata->db == NULL) {
		Tcl_AppendResult(interp,"dbi object has no open connection", NULL);
		return TCL_ERROR;
	}
	error = isc_drop_database(dbdata->status,&(dbdata->db));
	if (error) {
		dbi_Firebird_Error(interp,dbdata,"dropping database");
		return TCL_ERROR;
	}
	dbdata->db = NULL;
	error = dbi_Firebird_Close(dbdata);
	if (error) {return TCL_ERROR;}
	return TCL_OK;
}

int dbi_Firebird_Destroy(
	ClientData clientdata)
{
	dbi_Firebird_Data *dbdata = (dbi_Firebird_Data *)clientdata;
	Tcl_DecrRefCount(dbdata->defnullvalue);
	if (dbdata->database != NULL) {
		dbi_Firebird_Close(dbdata);
	}
	if (dbdata->in_sqlda != NULL) {
		Tcl_Free((char *)dbdata->in_sqlda);
	}
	if (dbdata->out_sqlda != NULL) {
		Tcl_Free((char *)dbdata->out_sqlda);
	}
	Tcl_Free((char *)dbdata);
	Tcl_DeleteExitHandler((Tcl_ExitProc *)dbi_Firebird_Destroy, clientdata);
	return TCL_OK;
}

int dbi_Firebird_Interface(
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	int i;
    static char *interfaces[] = {
		"dbi", DBI_VERSION, "dbi_admin", DBI_VERSION,
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
				Tcl_AppendResult(interp,interfaces[i+1],(char *) NULL);
				return TCL_OK;
			}
			i+=2;
		}
		Tcl_AppendResult(interp, Tcl_GetStringFromObj(objv[0],NULL), 
			" does not support interface ", interface, NULL);
		return TCL_ERROR;
	}
}

int Dbi_firebird_DbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_Firebird_Data *dbdata = (dbi_Firebird_Data *)clientdata;
	int error=TCL_OK,i,index;
    const static char *subCmds[] = {
		"interface","open", "exec", "fetch", "close",
		"info", "tables","fields",
		"begin", "commit", "rollback",
		"destroy", "serial","supports",
		"create", "drop","clone","clones","parent",
		"blob", "newblob",
		(char *) NULL};
    enum ISubCmdIdx {
		Interface, Open, Exec, Fetch, Close,
		Info, Tables, Fields,
		Begin, Commit, Rollback,
		Destroy, Serial, Supports,
		Create, Drop, Clone, Clones, Parent,
		Blob, Newblob
    };
	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "option ?...?");
		return TCL_ERROR;
	}
/*{int i;for (i = 0; i < objc; i++) {fprintf(stdout,"%s ",Tcl_GetStringFromObj(objv[i],NULL));};fflush(stdout);}*/
	error = Tcl_GetIndexFromObj(interp, objv[1], subCmds, "option", 0, (int *) &index);
	if (error != TCL_OK) {
		return error;
	}
    switch (index) {
    case Interface:
		return dbi_Firebird_Interface(interp,objc,objv);
    case Open:
		if (dbdata->parent != NULL) {
			Tcl_AppendResult(interp,"clone may not use open",NULL);
			return TCL_ERROR;
		}
		return dbi_Firebird_Open(interp,dbdata,objc,objv);
	case Create:
		return dbi_Firebird_Createdb(interp,dbdata,objc,objv);
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
				if (strncmp(Tcl_GetStringFromObj(dbcmd,NULL),"::dbi::firebird::priv_",23) == 0) continue;
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
		const static char *switches[] = {"-usefetch", "-nullvalue", "-flat", "-blobid","-cache", (char *) NULL};
	    enum switchesIdx {Usefetch, Nullvalue, Flat, Blobid, Cache};
		Tcl_Obj *nullvalue = NULL;
		char *string;
		int flags = 0;
		if (objc < 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "?options? command ?argument? ?...?");
			return TCL_ERROR;
		}
		i = 2;
		while (i < objc) {
			string = Tcl_GetStringFromObj(objv[i],NULL);
			if (string[0] != '-') break;
			if (Tcl_GetIndexFromObj(interp, objv[i], switches, "option", 0, &index)!= TCL_OK) {
				return TCL_ERROR;
			}
			switch (index) {
				case Usefetch:
					flags |= EXEC_USEFETCH;
					break;
				case Nullvalue:
					i++;
					if (i == objc) {
						Tcl_AppendResult(interp,"\"-nullvalue\" option must be followed by the value returned for NULL columns",NULL);
						return TCL_ERROR;
					}
					nullvalue = objv[i];
					break;
				case Flat:
					flags |= EXEC_FLAT;
					break;
				case Blobid:
					flags |= EXEC_BLOBID;
					break;
				case Cache:
					flags |= EXEC_CACHE;
					break;
			}
			i++;
		}
		return dbi_Firebird_Exec(interp,dbdata,objv[i],flags,nullvalue,objc-i-1,objv+i+1);
		}
	case Fetch:
		return dbi_Firebird_Fetch(interp,dbdata, objc, objv);
	case Info:
		return dbi_Firebird_Info(interp,dbdata,objc-2,objv+2);
	case Tables:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_Firebird_Tables(interp,dbdata);
	case Fields:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "tablename");
			return TCL_ERROR;
		}
		return dbi_Firebird_Cmd(interp,dbdata,objv[2],NULL,"::dbi::firebird::fieldsinfo");
	case Close:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->parent == NULL) {
			return dbi_Firebird_Close(dbdata);
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
		if (dbi_Firebird_trans_state(dbdata)) {
			error = dbi_Firebird_Transaction_Commit(interp,dbdata,1);
			if (error) {dbi_Firebird_Error(interp,dbdata,"committing transaction");return error;}
		}
		error = dbi_Firebird_Transaction_Start(interp,dbdata);
		if (error) {dbi_Firebird_Error(interp,dbdata,"starting transaction");return error;}
		dbi_Firebird_autocommit_set(dbdata,0);
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
		dbi_Firebird_autocommit_set(dbdata,1);
		if (dbdata->cursor_open) {dbi_Firebird_Free_Stmt(dbdata,DSQL_close);}
		if (dbi_Firebird_trans_state(dbdata)) {
			error = dbi_Firebird_Transaction_Commit(interp,dbdata,1);
			if (error) {dbi_Firebird_Error(interp,dbdata,"committing transaction");return error;}
		}
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
		dbi_Firebird_autocommit_set(dbdata,1);
		if (dbdata->cursor_open != 0) {dbi_Firebird_Free_Stmt(dbdata,DSQL_close);}
		if (dbi_Firebird_trans_state(dbdata)) {
			error = dbi_Firebird_Transaction_Rollback(interp,dbdata);
			if (error) {dbi_Firebird_Error(interp,dbdata,"rolling back transaction");return error;}
		}
		return TCL_OK;
	case Serial:
		if (objc < 5) {
			Tcl_WrongNumArgs(interp, 2, objv, "option table field ?value?");
			return TCL_ERROR;
		}
		return dbi_Firebird_Serial(interp,dbdata,objv[2],objc-3,objv+3);
	case Drop:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_Firebird_Dropdb(interp,dbdata);
	case Clone:
		if ((objc != 2) && (objc != 3)) {
			Tcl_WrongNumArgs(interp, 2, objv, "?name?");
			return TCL_ERROR;
		}
		if (objc == 3) {
			return Dbi_firebird_Clone(interp,dbdata,objv[2]);
		} else {
			return Dbi_firebird_Clone(interp,dbdata,NULL);
		}
	case Supports:
		if (objc == 2) {
			return dbi_Firebird_Supports(interp,dbdata,NULL);
		} if (objc == 3) {
			return dbi_Firebird_Supports(interp,dbdata,objv[2]);
		} else {
			Tcl_WrongNumArgs(interp, 2, objv, "?keyword?");
			return TCL_ERROR;
		}
	case Blob:
		return dbi_Firebird_Blob(interp,dbdata, objc, objv);
	case Newblob:
		return dbi_Firebird_NewBlob(interp,dbdata, objc, objv);
	}
	return error;
}

static int dbi_num = 0;
int Dbi_firebird_DoNewDbObjCmd(
	dbi_Firebird_Data *dbdata,
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
	dbdata->defnullvalue = Tcl_NewObj();
	Tcl_IncrRefCount(dbdata->defnullvalue);
	dbdata->dpb = NULL;
	dbdata->cursor_open = 0;
	dbdata->clones = NULL;
	dbdata->clonesnum = 0;
	dbdata->parent = NULL;
	dbdata->newblob_handle = NULL;
	memset(dbdata->newblob_buffer,0,SEGM_SIZE);
	dbdata->newblob_buffer_free = SEGM_SIZE;
	dbdata->blob_handle = NULL;
	/* 
	 * Allocate enough space for 20 fields.  
	 * If more fields get selected, re-allocate SQLDA later.
	 */
	Dbi_firebird_initsqlda(&(dbdata->out_sqlda));
	Dbi_firebird_initsqlda(&(dbdata->in_sqlda));
	if (dbi_nameObj == NULL) {
		dbi_num++;
		sprintf(buffer,"::dbi::firebird::dbi%d",dbi_num);
		dbi_nameObj = Tcl_NewStringObj(buffer,strlen(buffer));
	}
	dbi_name = Tcl_GetStringFromObj(dbi_nameObj,NULL);
	dbdata->token = Tcl_CreateObjCommand(interp,dbi_name,(Tcl_ObjCmdProc *)Dbi_firebird_DbObjCmd,
		(ClientData)dbdata,(Tcl_CmdDeleteProc *)dbi_Firebird_Destroy);
	Tcl_CreateExitHandler((Tcl_ExitProc *)dbi_Firebird_Destroy, (ClientData)dbdata);
	Tcl_SetObjResult(interp,dbi_nameObj);
	return TCL_OK;
}

int Dbi_firebird_NewDbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_Firebird_Data *dbdata;
	int error;
	if ((objc < 1)||(objc > 2)) {
		Tcl_WrongNumArgs(interp,2,objv,"?dbName?");
		return TCL_ERROR;
	}
	dbdata = (dbi_Firebird_Data *)Tcl_Alloc(sizeof(dbi_Firebird_Data));
	if (objc == 2) {
		error = Dbi_firebird_DoNewDbObjCmd(dbdata,interp,objv[1]);
	} else {
		error = Dbi_firebird_DoNewDbObjCmd(dbdata,interp,NULL);
	}
	if (error) {
		Tcl_Free((char *)dbdata);
	}
	return error;
}

int Dbi_firebird_Clone(
	Tcl_Interp *interp,
	dbi_Firebird_Data *dbdata,
	Tcl_Obj *name)
{
	dbi_Firebird_Data *parent = NULL;
	dbi_Firebird_Data *clone_dbdata = NULL;
	int error;
	parent = dbdata;
	while (parent->parent != NULL) {parent = parent->parent;}
	clone_dbdata = (dbi_Firebird_Data *)Tcl_Alloc(sizeof(dbi_Firebird_Data));
	error = Dbi_firebird_DoNewDbObjCmd(clone_dbdata,interp,name);
	if (error) {Tcl_Free((char *)clone_dbdata);return TCL_ERROR;}
	name = Tcl_GetObjResult(interp);
	parent->clonesnum++;
	parent->clones = (dbi_Firebird_Data **)Tcl_Realloc((char *)parent->clones,parent->clonesnum*sizeof(dbi_Firebird_Data **));
	parent->clones[parent->clonesnum-1] = clone_dbdata;
	clone_dbdata->parent = parent;
	clone_dbdata->dpblen = parent->dpblen;
	clone_dbdata->dpb = parent->dpb;
	clone_dbdata->database = parent->database;
	clone_dbdata->dpbpos = parent->dpbpos;
	clone_dbdata->db = parent->db;
	clone_dbdata->out_sqlda_cache = NULL;
	Tcl_InitHashTable(&(clone_dbdata->preparedhash),TCL_STRING_KEYS);
	error = isc_dsql_allocate_statement(clone_dbdata->status, &(clone_dbdata->db), &(clone_dbdata->stmt));
	if (error) {
		dbi_Firebird_Error(interp,dbdata,"allocating statement");
		goto error;
	}
	return TCL_OK;
	error:
		Tcl_DeleteCommandFromToken(interp,clone_dbdata->token);
		Tcl_Free((char *)clone_dbdata);
		return TCL_ERROR;
}

int Dbi_firebird_Init(interp)
	Tcl_Interp *interp;		/* Interpreter to add extra commands */
{
#ifdef USE_TCL_STUBS
	if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
		return TCL_ERROR;
	}
#endif
	Tcl_CreateObjCommand(interp,"dbi_firebird",(Tcl_ObjCmdProc *)Dbi_firebird_NewDbObjCmd,
		(ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
	Tcl_Eval(interp,"");
	return TCL_OK;
}
