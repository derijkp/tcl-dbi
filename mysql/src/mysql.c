/*
 *       File:    mem.c
 *       Purpose: dbi extension to Tcl: mysql backend
 *       Author:  Copyright (c) 1998 Peter De Rijk
 *
 *       See the file "README" for information on usage and redistribution
 *       of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tcl.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "dbi_mysql.h"

void Tcl_GetCommandFullName(
    Tcl_Interp *interp,
    Tcl_Command command,
    Tcl_Obj *objPtr);

int Dbi_mysql_Clone(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	Tcl_Obj *name);

/******************************************************************/

int dbi_Mysql_Error(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	char *premsg)
{
	if (strlen(premsg) != 0) {
		Tcl_AppendResult(interp,"error ",premsg,":\n", NULL);
	}
	Tcl_AppendResult(interp,"error ",mysql_error(dbdata->db),NULL);
	return TCL_ERROR;
}

int dbi_Mysql_TclEval(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
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

int dbi_Mysql_connected(
	dbi_Mysql_Data *dbdata
)
{
	return (dbdata->database != NULL);
}

int dbi_Mysql_Open(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	int objc,
	Tcl_Obj *objv[]
)
{
	static char *switches[] = {"-user","-password","-host","-ssl", (char *) NULL};
	char *host=NULL,*user=NULL,*password=NULL;
	unsigned int client_flag = 0;
	int len,i,index;
	if (dbi_Mysql_connected(dbdata)) {
		Tcl_AppendResult(interp,"dbi object has open connection, close first", NULL);
		goto error;
	}
	if (objc < 3) {
		Tcl_WrongNumArgs(interp,2,objv,"datasource ?options?");
		return TCL_ERROR;
	}
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
				user = Tcl_GetStringFromObj(objv[i+1], &len);
				i++;
				break;
			case 1:			/* -password */
				if (i == (objc-1)) {
					Tcl_AppendResult(interp,"\"-password\" option must be followed by the password of the user",NULL);
					return TCL_ERROR;
				}
				password = Tcl_GetStringFromObj(objv[i+1], &len);
				i++;
				break;
			case 2:			/* -host */
				if (i == (objc-1)) {
					Tcl_AppendResult(interp,"\"-role\" option must be followed by the role",NULL);
					return TCL_ERROR;
				}
				host = Tcl_GetStringFromObj(objv[i+1], &len);
				i++;
				break;
			case 3:			/* -ssl */
				client_flag |= CLIENT_SSL;
				i++;
				break;
		}
	}
	dbdata->database = objv[2];
	Tcl_IncrRefCount(dbdata->database);
	if (!mysql_real_connect(dbdata->db,host,user,password,
		Tcl_GetStringFromObj(dbdata->database,NULL),0,NULL,client_flag))
	{
		Tcl_AppendResult(interp,"connection to database \"",
			Tcl_GetStringFromObj(dbdata->database,NULL) , "\" failed:\n", NULL);
		Tcl_DecrRefCount(dbdata->database);dbdata->database=NULL;
		dbi_Mysql_Error(interp,dbdata,"opening to database");
		goto error;
	}
	return TCL_OK;
	error:
		return TCL_ERROR;
}

int dbi_Mysql_Find_Prev(
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

int dbi_Mysql_Clearresult(
	dbi_Mysql_Data *dbdata
)
{
	if (dbdata->result != NULL) {mysql_free_result(dbdata->result); dbdata->result = NULL;}
	dbdata->ntuples = 0;
	dbdata->nfields = 0;
	dbdata->row = NULL;
	return TCL_OK;
}

int dbi_Mysql_GetOne(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	int ituple,
	int ifield,
	Tcl_Obj *nullvalue,
	Tcl_Obj **result)
{
	MYSQL_FIELD *fieldinfo;
	char *string;
	double d;
	unsigned long *lengths;
	int len,type;
	if ((ituple != -1) && (ituple != dbdata->tuple)) {
		mysql_data_seek(dbdata->result, (unsigned long long)ituple);
		dbdata->tuple = ituple;
		dbdata->row = mysql_fetch_row(dbdata->result);
	}
	if (dbdata->row[ifield] == NULL) {
		*result = nullvalue;
	} else {
		fieldinfo = mysql_fetch_field_direct(dbdata->result,ifield);
		lengths = mysql_fetch_lengths(dbdata->result);
		string = dbdata->row[ifield];
		len = lengths[ifield];
		type = fieldinfo->type;
		*result = Tcl_NewStringObj(string,len);
		switch (type) {
			case FIELD_TYPE_FLOAT:
			case FIELD_TYPE_DOUBLE:
				Tcl_GetDoubleFromObj(interp,*result,&d);
				Tcl_SetDoubleObj(*result, d);
				break;
			case FIELD_TYPE_TIME:
				Tcl_AppendToObj(*result,".000",4);
				break;
			case FIELD_TYPE_TIMESTAMP:
				{
				char buffer[24];
				int len;
				string = Tcl_GetStringFromObj(*result,&len);
				switch (len) {
					case 2:
						sprintf(buffer,"%2.2s",string);
						break;
					case 4:
						sprintf(buffer,"%2.2s-%2.2s",string,string+2);
						break;
					case 6:
						printf(buffer,"%2.2s-%2.2s-%2.2s",string,string+2,string+4);
						break;
					case 8:
						sprintf(buffer,"%4.4s-%2.2s-%2.2s",string,string+4,string+6);
						break;
					case 10:
						sprintf(buffer,"%2.2s-%2.2s-%2.2s %2.2s:%2.2s",
							string,string+2,string+4,string+6,string+8);
						break;
					case 12:
						sprintf(buffer,"%2.2s-%2.2s-%2.2s %2.2s:%2.2s:%2.2s.000",
							string,string+2,string+4,string+6,string+8,string+10);
						break;
					case 14:
						sprintf(buffer,"%4.4s-%2.2s-%2.2s %2.2s:%2.2s:%2.2s.000",
							string,string+4,string+6,string+8,string+10,string+12);
						break;
				}
				Tcl_SetStringObj(*result,buffer,-1);
				break;
				}
			default:
				break;
		}
	}
	return TCL_OK;
}

int dbi_Mysql_Fetch_Row(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	Tcl_Obj *nullvalue,
	Tcl_Obj **result)
{
	Tcl_Obj *line = NULL, *element = NULL;
	unsigned int num_fields;
	unsigned long *lengths;
	int i,error;
	num_fields = dbdata->nfields;
	lengths = mysql_fetch_lengths(dbdata->result);
	line = Tcl_NewListObj(0,NULL);
	for(i = 0; i < num_fields; i++) {
		error = dbi_Mysql_GetOne(interp,dbdata,-1,i,nullvalue,&element);
		if (error) {goto error;}
		error = Tcl_ListObjAppendElement(interp, line, element);
		if (error) {goto error;}
		element = NULL;
	}
	*result = line;
	return TCL_OK;
	error:
		if (line != NULL) Tcl_DecrRefCount(line);
		if (element != NULL) Tcl_DecrRefCount(element);
		return TCL_ERROR;
}

int dbi_Mysql_ToResult(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	Tcl_Obj *nullvalue)
{
	Tcl_Obj *result = NULL, *line = NULL;
	int error;
	if (nullvalue == NULL) {nullvalue = dbdata->defnullvalue;}
	result = Tcl_NewListObj(0,NULL);
	while (1) {
		dbdata->row = mysql_fetch_row(dbdata->result);
		if (!dbdata->row) break;
		error = dbi_Mysql_Fetch_Row(interp,dbdata,nullvalue,&line);
		if (error) {goto error;}
		error = Tcl_ListObjAppendElement(interp, result, line);
		if (error) goto error;
		line = NULL;
	}
	Tcl_SetObjResult(interp, result);
	return TCL_OK;
	mysql_free_result(dbdata->result); dbdata->result = NULL;
	error:
		mysql_free_result(dbdata->result); dbdata->result = NULL;
		if (result != NULL) Tcl_DecrRefCount(result);
		if (line != NULL) Tcl_DecrRefCount(line);
		return TCL_ERROR;
}

int dbi_Mysql_ToResult_flat(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	Tcl_Obj *nullvalue)
{
	Tcl_Obj *result = NULL, *element = NULL;
	unsigned int num_fields;
	unsigned long *lengths;
	int error,i;
	if (nullvalue == NULL) {nullvalue = dbdata->defnullvalue;}
	result = Tcl_NewListObj(0,NULL);
	num_fields = dbdata->nfields;
	lengths = mysql_fetch_lengths(dbdata->result);
	while (1) {
		dbdata->row = mysql_fetch_row(dbdata->result);
		if (!dbdata->row) break;
		lengths = mysql_fetch_lengths(dbdata->result);
		for(i = 0; i < num_fields; i++) {
			error = dbi_Mysql_GetOne(interp,dbdata,-1,i,nullvalue,&element);
			if (error) {goto error;}
			error = Tcl_ListObjAppendElement(interp,result,element);
			if (error) goto error;
			element = NULL;
		}
	}
	Tcl_SetObjResult(interp, result);
	mysql_free_result(dbdata->result); dbdata->result = NULL;
	return TCL_OK;
	error:
		mysql_free_result(dbdata->result); dbdata->result = NULL;
		if (result != NULL) Tcl_DecrRefCount(result);
		if (element != NULL) Tcl_DecrRefCount(element);
		return TCL_ERROR;
}

int dbi_Mysql_Fetch_Array(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	Tcl_Obj *nullvalue,
	Tcl_Obj **result)
{
	MYSQL_FIELD *fields = mysql_fetch_fields(dbdata->result);
	unsigned int num_fields = dbdata->nfields;
	Tcl_Obj *line = NULL, *element = NULL;
	int i,error;
	line = Tcl_NewListObj(0,NULL);
	for (i = 0; i < num_fields; i++) {
		element = Tcl_NewStringObj(fields[i].name,-1);
		error = Tcl_ListObjAppendElement(interp,line, element);
		if (error) goto error;
		element = NULL;
		error = dbi_Mysql_GetOne(interp,dbdata,-1,i,nullvalue,&element);
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

int dbi_Mysql_Fetch(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	Tcl_Obj *element;
	Tcl_Obj *tuple = NULL, *field = NULL, *nullvalue = NULL, *line = NULL;
	int fetch_option,ituple=-1,ifield=-1;
	int error;
    static char *subCmds[] = {
		 "data", "lines", "pos", "fields", "isnull", "array",
		(char *) NULL};
    enum ISubCmdIdx {
		Data, Lines, Pos, Fields, Isnull, Array
    };
    static char *switches[] = {
		"-nullvalue",
		(char *) NULL};
    enum switchesIdx {
		Nullvalue,
    };
	if (dbdata->result == NULL) {
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
		ifield = -1;
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
			Tcl_SetObjResult(interp,Tcl_NewIntObj((int)dbdata->ntuples));
			return TCL_OK;
		case Fields:
			{
			MYSQL_FIELD *fields = mysql_fetch_fields(dbdata->result);
			unsigned int num_fields = dbdata->nfields;
			int i;
			for (i = 0 ; i < num_fields ; i++) {
				Tcl_AppendElement(interp,fields[i].name);
			}
			}
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
	if ((dbdata->ntuples != -1)&&(ituple >= dbdata->ntuples)) {
		goto out_of_position;
	}
	if (ituple == (dbdata->tuple + 1)) {
		dbdata->row = mysql_fetch_row(dbdata->result);
		dbdata->tuple = ituple;
	} else if (ituple != dbdata->tuple) {
		mysql_data_seek(dbdata->result, (unsigned long long)ituple);
		dbdata->row = mysql_fetch_row(dbdata->result);
		dbdata->tuple = ituple;
	}
	if (ifield >= dbdata->nfields) {
		Tcl_Obj *buffer;
		buffer = Tcl_NewIntObj(ifield);
		Tcl_AppendResult(interp, "field ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
		Tcl_DecrRefCount(buffer);
		return TCL_ERROR;
	}
	if (nullvalue == NULL) {
		nullvalue = dbdata->defnullvalue;
	}
	switch (fetch_option) {
		case Data:
			if (nullvalue == NULL) {
				nullvalue = dbdata->defnullvalue;
			}
			if (ifield == -1) {
				error = dbi_Mysql_Fetch_Row(interp,dbdata,nullvalue,&line);
				if (error) {return error;}
				Tcl_SetObjResult(interp, line);
				line = NULL;
			} else {
				dbi_Mysql_GetOne(interp,dbdata,-1,ifield,nullvalue,&element);
				Tcl_SetObjResult(interp, element);
				line = NULL;
			}
			break;
		case Isnull:
			if (dbdata->row[ifield] == NULL) {
				Tcl_SetObjResult(interp,Tcl_NewIntObj(1));
			} else {
				Tcl_SetObjResult(interp,Tcl_NewIntObj(0));
			}
			return TCL_OK;
		case Array:
			error = dbi_Mysql_Fetch_Array(interp,dbdata,nullvalue,&line);
			if (error) {return error;}
			Tcl_SetObjResult(interp, line);
			break;
	}
	return TCL_OK;
	out_of_position:
		{
		Tcl_Obj *buffer;
		buffer = Tcl_NewIntObj(dbdata->ntuples);
		Tcl_AppendResult(interp, "line ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
		Tcl_DecrRefCount(buffer);
		return TCL_ERROR;
		}
	return TCL_OK;
}

int dbi_Mysql_Transaction_Commit(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	int noretain)
{
	int error;
	error = mysql_real_query(dbdata->db,"commit",6);
	if (error) {return TCL_ERROR;}
	return TCL_OK;
}

int dbi_Mysql_Transaction_Rollback(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata)
{
	int error;
	error = mysql_real_query(dbdata->db,"rollback",8);
	if (error) {return TCL_ERROR;}
	return TCL_OK;
}

int dbi_Mysql_Transaction_Start(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata)
{
	int error;
	error = mysql_real_query(dbdata->db,"begin",5);
	if (error) {return TCL_ERROR;}
	return TCL_OK;
}

int dbi_Mysql_ParseStatement(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	char *src,
	int len,
	int *parsedstatement[],
	int *resultsize)
{
	int *result;
	int pos=0;
	int i;
	result = (int *)Tcl_Alloc(sizeof(int));
	for (i=0 ; i < len ; i++) {
		if (src[i] == '/') {
			i++;
			if (i == len) break;
			/* C style comment */
			if (src[i] == '*') {
				while (i<len) {
					i++;
					if ((src[i] == '*')&&(src[i+1] == '/')) break;
				}
			}
			/* C++ style comment */
			if (src[i] == '/') {
				while (i<len) {
					i++;
					if ((src[i] == '\n')||(src[i] == '/')) break;
				}
			}
		} else if ((src[i] == '\'')||(src[i] == '\"')) {
			char delim = src[i];
			/* literal */
			if (src[i] == delim) {
				while (i<len) {
					i++;
					if (src[i] == '\\') {
						i++;
					} else if (src[i] == delim) break;
				}
			}
		} else if (src[i] == '?') {
			result[pos++] = i;
			result = (int *)Tcl_Realloc((char *)result,(pos+1)*sizeof(int));
		}
	}
	result[pos] = -1;
	*resultsize = pos;
	*parsedstatement = result;
	return TCL_OK;
}

int dbi_Mysql_SplitSQL(
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
			if (dbi_Mysql_Find_Prev(cmdstring+prevline,i-prevline,"begin")) {
				blevel++;
			} else if (dbi_Mysql_Find_Prev(cmdstring+prevline,i-prevline,"BEGIN")) {
				blevel++;
			} else if (dbi_Mysql_Find_Prev(cmdstring+prevline,i-prevline,"end")) {
				blevel--;
			} else if (dbi_Mysql_Find_Prev(cmdstring+prevline,i-prevline,"END")) {
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
			if (dbi_Mysql_Find_Prev(cmdstring+prevline,i-prevline,"end")) {
				blevel--;
			} else if (dbi_Mysql_Find_Prev(cmdstring+prevline,i-prevline,"END")) {
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

int dbi_Mysql_InsertParam(
	Tcl_Interp *interp,
	char *cmdstring,
	int cmdlen,
	int *parsedstatement,
	char *nullstring,
	int nulllen,
	Tcl_Obj **objv,
	int objc,
	char **result,
	int *resultlen
)
{
	char *sqlstring,*cursqlstring,*argstring;
	int i,size,prvsrcpos,arglen;
	/* calculate size of buffer needed */
	size = cmdlen;
	for (i = 0 ; i < objc ; i++) {
		Tcl_GetStringFromObj(objv[i],&arglen);
		/* we need at least space for 4 characters: in case of a NULL */
		if (arglen < 2) {arglen = 2;}
		/* add 2 for quotes around the value, and 4 more as a reserve for quotes in the value */
		size += arglen+6;
	}
	sqlstring = (char *)Tcl_Alloc(size*sizeof(char));
	cursqlstring = sqlstring;
	prvsrcpos = 0;
	/* put sql with inserted parameters into buffer */
	for (i = 0 ; i < objc ; i++) {
		strncpy(cursqlstring,cmdstring+prvsrcpos,parsedstatement[i]-prvsrcpos);
		cursqlstring += parsedstatement[i]-prvsrcpos;
		prvsrcpos = parsedstatement[i]+1;
		argstring = Tcl_GetStringFromObj(objv[i],&arglen);
		if ((nullstring != NULL)&&(arglen == nulllen)&&(strncmp(argstring,nullstring,arglen) == 0)) {
			strncpy(cursqlstring,"NULL",4);
			cursqlstring += 4;
		} else {
			char *pos;
			int partsize,count=0,curpos;
			*cursqlstring++ = '\'';
			while (1) {
				pos = strchr(argstring,'\'');
				if (pos == NULL) {
					strncpy(cursqlstring,argstring,arglen);
					cursqlstring += arglen;
					break;
				} else {
					count++;
					if (count > 4) {
						size += 1;
						curpos = cursqlstring - sqlstring;
						sqlstring = (char *)Tcl_Realloc(sqlstring,size*sizeof(char));
						cursqlstring = sqlstring+curpos;
					}
					partsize = pos-argstring;
					strncpy(cursqlstring,argstring,partsize);
					cursqlstring += pos-argstring;
					*cursqlstring++ = '\'';
					*cursqlstring++ = '\'';
					arglen -= partsize+1;
					argstring = pos+1;
				}
			}
			*cursqlstring++ = '\'';
		}
	}
	Tcl_ResetResult(interp);
	strncpy(cursqlstring,cmdstring+prvsrcpos,cmdlen-prvsrcpos);
	cursqlstring[cmdlen-prvsrcpos] = '\0';
	Tcl_Free((char *)parsedstatement);
	*result = sqlstring;
	if (resultlen != NULL) {*resultlen = cursqlstring-sqlstring+cmdlen-prvsrcpos;}
	return TCL_OK;
}

int dbi_Mysql_Exec(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	Tcl_Obj *cmd,
	int flags,
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	char *cmdstring = NULL, **lines = NULL;
	char *sqlstring = NULL;
	int *parsedstatement = NULL;
	int error,cmdlen,num,numargs;
	int flat, usefetch, cache,*sizes = NULL;
	flat = flags & EXEC_FLAT;
	usefetch = flags & EXEC_USEFETCH;
	cache = flags & EXEC_CACHE;
	if (dbdata->db == NULL) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		return TCL_ERROR;
	}
	cmdstring = Tcl_GetStringFromObj(cmd,&cmdlen);
	if (cmdlen == 0) {return TCL_OK;}
	dbi_Mysql_Clearresult(dbdata);
	error = dbi_Mysql_SplitSQL(interp,cmdstring,cmdlen,&lines,&sizes);
	if (error) {goto error;}
	if (lines == NULL) {
		error = dbi_Mysql_ParseStatement(interp,dbdata,cmdstring,cmdlen,&parsedstatement,&numargs);
		if (objc != numargs) {
			Tcl_Free((char *)parsedstatement);parsedstatement=NULL;
			Tcl_AppendResult(interp,"wrong number of arguments given to exec", NULL);
			Tcl_AppendResult(interp," while executing command: \"",	cmdstring, "\"", NULL);
			return TCL_ERROR;
		}
		if (numargs > 0) {
			char *nullstring = NULL;
			int sqlstringlen,nulllen;
			if (nullvalue == NULL) {
				nullstring = NULL; nulllen = 0;
				dbdata->nullvalue = 	dbdata->defnullvalue;
			} else {
				nullstring = Tcl_GetStringFromObj(nullvalue,&nulllen);
				dbdata->nullvalue = nullvalue;
			}
			if (usefetch) {
				dbdata->nullvalue = 	dbdata->defnullvalue;
			}
			error = dbi_Mysql_InsertParam(interp,cmdstring,cmdlen,parsedstatement,
				nullstring,nulllen,objv,objc,&sqlstring,&sqlstringlen);
			if (error) {goto error;}
			/*printf("sql = %s\n",sqlstring);*/
			error =  mysql_real_query(dbdata->db,sqlstring,sqlstringlen);
			Tcl_Free(sqlstring);sqlstring=NULL;
		} else {
			error =  mysql_real_query(dbdata->db,cmdstring,cmdlen);
		}
		if (parsedstatement == NULL) {Tcl_Free((char *)parsedstatement);parsedstatement=NULL;}
		if (error) {dbi_Mysql_Error(interp,dbdata,"");goto error;}
	} else {
		if (objc != 0) {
			Tcl_AppendResult(interp,"multiple commands cannot take arguments\n", NULL);
			goto error;
		}
		num = 0;
		while (lines[num] != NULL) {
			error =  mysql_real_query(dbdata->db,lines[num],sizes[num]);
			if (error) {dbi_Mysql_Error(interp,dbdata,"");goto error;}
			num++;
		}
		Tcl_Free((char *)lines);lines = NULL;
		Tcl_Free((char *)sizes);sizes = NULL;
	}
	dbdata->result = mysql_store_result(dbdata->db);
	if (dbdata->result) {
		dbdata->ntuples = mysql_num_rows(dbdata->result);
		dbdata->nfields = mysql_num_fields(dbdata->result);
		if (!usefetch) {
			if (flat) {
				error = dbi_Mysql_ToResult_flat(interp,dbdata,nullvalue);
				dbi_Mysql_Clearresult(dbdata);
			} else {
				error = dbi_Mysql_ToResult(interp,dbdata,nullvalue);
				dbi_Mysql_Clearresult(dbdata);
			}
			if (error) {goto error;}
		} else {
			dbdata->tuple = -1;
		}
	} else {
		if (mysql_errno(dbdata->db)) {
			goto error;
		}
	}
	return TCL_OK;
	error:
		{
		Tcl_Obj *temp;
		temp = Tcl_NewStringObj(cmdstring,cmdlen);
		Tcl_AppendResult(interp," while executing command: \"",	Tcl_GetStringFromObj(temp,NULL), "\"", NULL);
		Tcl_DecrRefCount(temp);
		if (lines != NULL) {Tcl_Free((char *)lines);}
		if (sizes != NULL) {Tcl_Free((char *)sizes);}
		if (parsedstatement != NULL) {Tcl_Free((char *)parsedstatement);}
		return TCL_ERROR;
		}
}

int dbi_Mysql_Supports(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	Tcl_Obj *keyword)
{
	static char *keywords[] = {
		"lines","backfetch","serials","sharedserials","blobparams","blobids",
		"transactions","sharedtransactions","foreignkeys","checks","views",
		"columnperm","roles","domains","permissions",
		(char *) NULL};
	static int supports[] = {
		1,1,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0};
	enum keywordsIdx {
		Lines,Backfetch,Serials,Sharedserials,Blobparams, Blobids,
		Transactions,Sharedtransactions,Foreignkeys,Checks,Views,
		Columnperm, Roles, Domains, Permissions
	};
	int error,index;
	if (keyword == NULL) {
		char **keyword = keywords;
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

int dbi_Mysql_Info(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	int objc,
	Tcl_Obj **objv)
{
	Tcl_Obj *cmd,*dbcmd;
	int error,i;
	dbcmd = Tcl_NewObj();
	Tcl_GetCommandFullName(interp, dbdata->token, dbcmd);
	cmd = Tcl_NewStringObj("::dbi::mysql::info",-1);
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

int dbi_Mysql_Tables(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata)
{
	int error;
	Tcl_Obj *cmd = Tcl_NewStringObj("show tables",-1);
	error = dbi_Mysql_Exec(interp,dbdata,cmd,EXEC_FLAT,NULL,0,NULL);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Mysql_Cmd(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
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

int dbi_Mysql_Serial(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	Tcl_Obj *subcmd,
	int objc,
	Tcl_Obj **objv)
{
	int error,index;
    static char *subCmds[] = {
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
			error = dbi_Mysql_TclEval(interp,dbdata,"::dbi::mysql::serial_add",objc,objv);	
			break;
		case Delete:
			error = dbi_Mysql_TclEval(interp,dbdata,"::dbi::mysql::serial_delete",objc,objv);	
			break;
		case Set:
			error = dbi_Mysql_TclEval(interp,dbdata,"::dbi::mysql::serial_set",objc,objv);	
			break;
		case Next:
			error = dbi_Mysql_TclEval(interp,dbdata,"::dbi::mysql::serial_next",objc,objv);	
			break;
		case Share:
			error = dbi_Mysql_TclEval(interp,dbdata,"::dbi::mysql::serial_share",objc,objv);	
			break;
	}
	return error;
}

int dbi_Mysql_Createdb(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	return TCL_OK;
}

int dbi_Mysql_Close(
	dbi_Mysql_Data *dbdata)
{
	while (dbdata->clonesnum) {
		Tcl_DeleteCommandFromToken(dbdata->interp,dbdata->clones[0]->token);
	}
	if (dbdata->clones != NULL) {
		Tcl_Free((char *)dbdata->clones);
		dbdata->clones = NULL;
	}
	if (dbdata->parent == NULL) {
		/* this is not a clone, so really close */
		if (dbdata->database != NULL) {
			Tcl_DecrRefCount(dbdata->database);dbdata->database = NULL;
		}
		if (dbi_Mysql_connected(dbdata)) {
			mysql_close(dbdata->db);
		}
		if (dbdata->result != NULL) {mysql_free_result(dbdata->result); dbdata->result = NULL;}
	} else {
		/* this is a clone, so remove the clone from its parent */
		dbi_Mysql_Data *parent = dbdata->parent;
		int i;
		for (i = 0 ; i < parent->clonesnum; i++) {
			if (parent->clones[i] == dbdata) break;
		}
		i++;
		for (; i < parent->clonesnum; i++) {
			parent->clones[i-1] = parent->clones[i];
		}
		parent->clonesnum--;
		if (dbdata->result != NULL) {mysql_free_result(dbdata->result); dbdata->result = NULL;}
	}
	return TCL_OK;
}

int dbi_Mysql_Dropdb(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata)
{
	Tcl_Obj *database;
	int error;
	if (dbdata->parent != NULL) {
		Tcl_AppendResult(interp,"cannot drop database from a clone", NULL);
		return TCL_ERROR;
	}
	if (!dbi_Mysql_connected(dbdata)) {
		Tcl_AppendResult(interp,"dbi object has no open connection", NULL);
		return TCL_ERROR;
	}
	database = dbdata->database;
	Tcl_IncrRefCount(database);
	error = dbi_Mysql_Close(dbdata);
	if (error) {Tcl_DecrRefCount(database);return error;}
	error = Tcl_VarEval(interp,"file delete ",Tcl_GetStringFromObj(database,NULL),NULL);
	Tcl_DecrRefCount(database);
	if (error) {return error;}
	return TCL_OK;
}

int dbi_Mysql_Destroy(
	ClientData clientdata)
{
	dbi_Mysql_Data *dbdata = (dbi_Mysql_Data *)clientdata;
	if (dbdata->database != NULL) {
		dbi_Mysql_Close(dbdata);
	}
	if (dbdata->parent == NULL) {
		/* this is not a clone, so really remove defnullvalue */
		Tcl_DecrRefCount(dbdata->defnullvalue);
	}
	Tcl_Free((char *)dbdata);
	Tcl_DeleteExitHandler((Tcl_ExitProc *)dbi_Mysql_Destroy, clientdata);
	return TCL_OK;
}

int dbi_Mysql_Interface(
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

int Dbi_mysql_DbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_Mysql_Data *dbdata = (dbi_Mysql_Data *)clientdata;
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
		Create, Drop, Clone, Clones, Parent,
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
		return dbi_Mysql_Interface(interp,objc,objv);
    case Open:
		if (dbdata->parent != NULL) {
			Tcl_AppendResult(interp,"clone may not use open",NULL);
			return TCL_ERROR;
		}
		return dbi_Mysql_Open(interp,dbdata,objc,objv);
	case Create:
		return dbi_Mysql_Createdb(interp,dbdata,objc,objv);
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
				if (strncmp(Tcl_GetStringFromObj(dbcmd,NULL),"::dbi::mysql::priv_",23) == 0) continue;
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
	if (!dbi_Mysql_connected(dbdata)) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		return TCL_ERROR;
	}
    switch (index) {
	case Exec:
		{
	    static char *switches[] = {"-usefetch", "-nullvalue", "-flat","-cache", (char *) NULL};
	    enum switchesIdx {Usefetch, Nullvalue, Flat, Cache};
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
			}
			i++;
		}
		return dbi_Mysql_Exec(interp,dbdata,objv[i],flags,nullvalue,objc-i-1,objv+i+1);
		}
	case Fetch:
		return dbi_Mysql_Fetch(interp,dbdata, objc, objv);
	case Info:
		return dbi_Mysql_Info(interp,dbdata,objc-2,objv+2);
	case Tables:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_Mysql_Tables(interp,dbdata);
	case Fields:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "tablename");
			return TCL_ERROR;
		}
		return dbi_Mysql_Cmd(interp,dbdata,objv[2],NULL,"::dbi::mysql::fieldsinfo");
	case Close:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->parent == NULL) {
			return dbi_Mysql_Close(dbdata);
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
		if (!dbi_Mysql_connected(dbdata)) {
			Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
			return error;
		}
		error = dbi_Mysql_Transaction_Commit(interp,dbdata,1);
		if (error) {dbi_Mysql_Error(interp,dbdata,"committing transaction");return error;}
		error = dbi_Mysql_Transaction_Start(interp,dbdata);
		if (error) {dbi_Mysql_Error(interp,dbdata,"starting transaction");return error;}
		return TCL_OK;
	case Commit:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (!dbi_Mysql_connected(dbdata)) {
			Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
			return error;
		}
		error = dbi_Mysql_Transaction_Commit(interp,dbdata,1);
		if (error) {dbi_Mysql_Error(interp,dbdata,"committing transaction");return error;}
		return TCL_OK;
	case Rollback:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (!dbi_Mysql_connected(dbdata)) {
			Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
			return error;
		}
		error = dbi_Mysql_Transaction_Rollback(interp,dbdata);
		if (error) {dbi_Mysql_Error(interp,dbdata,"rolling back transaction");return error;}
		return TCL_OK;
	case Serial:
		if (objc < 5) {
			Tcl_WrongNumArgs(interp, 2, objv, "option table field ?value?");
			return TCL_ERROR;
		}
		return dbi_Mysql_Serial(interp,dbdata,objv[2],objc-3,objv+3);
	case Drop:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_Mysql_Dropdb(interp,dbdata);
	case Clone:
		if ((objc != 2) && (objc != 3)) {
			Tcl_WrongNumArgs(interp, 2, objv, "?name?");
			return TCL_ERROR;
		}
		if (objc == 3) {
			return Dbi_mysql_Clone(interp,dbdata,objv[2]);
		} else {
			return Dbi_mysql_Clone(interp,dbdata,NULL);
		}
	case Supports:
		if (objc == 2) {
			return dbi_Mysql_Supports(interp,dbdata,NULL);
		} if (objc == 3) {
			return dbi_Mysql_Supports(interp,dbdata,objv[2]);
		} else {
			Tcl_WrongNumArgs(interp, 2, objv, "?keyword?");
			return TCL_ERROR;
		}
	}
	return error;
}

static int dbi_num = 0;
int Dbi_mysql_DoNewDbObjCmd(
	dbi_Mysql_Data *dbdata,
	Tcl_Interp *interp,
	Tcl_Obj *dbi_nameObj)
{
	char buffer[40];
	char *dbi_name;
	dbdata->interp = interp;
	dbdata->db = mysql_init(NULL);
	dbdata->database=NULL;
	dbdata->result=NULL;
	dbdata->defnullvalue = NULL;
	dbdata->nullvalue = NULL;
	dbdata->clones = NULL;
	dbdata->clonesnum = 0;
	dbdata->parent = NULL;
	if (dbi_nameObj == NULL) {
		dbi_num++;
		sprintf(buffer,"::dbi::mysql::dbi%d",dbi_num);
		dbi_nameObj = Tcl_NewStringObj(buffer,strlen(buffer));
	}
	dbi_name = Tcl_GetStringFromObj(dbi_nameObj,NULL);
	dbdata->token = Tcl_CreateObjCommand(interp,dbi_name,(Tcl_ObjCmdProc *)Dbi_mysql_DbObjCmd,
		(ClientData)dbdata,(Tcl_CmdDeleteProc *)dbi_Mysql_Destroy);
	Tcl_CreateExitHandler((Tcl_ExitProc *)dbi_Mysql_Destroy, (ClientData)dbdata);
	Tcl_SetObjResult(interp,dbi_nameObj);
	return TCL_OK;
}

int Dbi_mysql_NewDbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_Mysql_Data *dbdata;
	int error;
	if ((objc < 1)||(objc > 2)) {
		Tcl_WrongNumArgs(interp,2,objv,"?dbName?");
		return TCL_ERROR;
	}
	dbdata = (dbi_Mysql_Data *)Tcl_Alloc(sizeof(dbi_Mysql_Data));
	if (objc == 2) {
		error = Dbi_mysql_DoNewDbObjCmd(dbdata,interp,objv[1]);
	} else {
		error = Dbi_mysql_DoNewDbObjCmd(dbdata,interp,NULL);
	}
	dbdata->defnullvalue = Tcl_NewObj();
	Tcl_IncrRefCount(dbdata->defnullvalue);
	if (error) {
		Tcl_Free((char *)dbdata);
	}
	return error;
}

int Dbi_mysql_Clone(
	Tcl_Interp *interp,
	dbi_Mysql_Data *dbdata,
	Tcl_Obj *name)
{
	dbi_Mysql_Data *parent = NULL;
	dbi_Mysql_Data *clone_dbdata = NULL;
	int error;
	parent = dbdata;
	while (parent->parent != NULL) {parent = parent->parent;}
	clone_dbdata = (dbi_Mysql_Data *)Tcl_Alloc(sizeof(dbi_Mysql_Data));
	error = Dbi_mysql_DoNewDbObjCmd(clone_dbdata,interp,name);
	if (error) {Tcl_Free((char *)clone_dbdata);return TCL_ERROR;}
	parent->clonesnum++;
	parent->clones = (dbi_Mysql_Data **)Tcl_Realloc((char *)parent->clones,parent->clonesnum*sizeof(dbi_Mysql_Data **));
	parent->clones[parent->clonesnum-1] = clone_dbdata;
	clone_dbdata->parent = parent;
	clone_dbdata->db = parent->db;
	clone_dbdata->database = parent->database;
	clone_dbdata->result = NULL;
	clone_dbdata->nullvalue = NULL;
	clone_dbdata->defnullvalue = parent->defnullvalue;
	return TCL_OK;
}

int Dbi_mysql_Init(interp)
	Tcl_Interp *interp;		/* Interpreter to add extra commands */
{
#ifdef USE_TCL_STUBS
	if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
		return TCL_ERROR;
	}
#endif
	Tcl_CreateObjCommand(interp,"dbi_mysql",(Tcl_ObjCmdProc *)Dbi_mysql_NewDbObjCmd,
		(ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
	Tcl_Eval(interp,"");
	return TCL_OK;
}
