/*
 *	   File:	odbc.c
 *	   Purpose: dbi extension to Tcl: odbc backend
 *	   Author:  Copyright (c) 1998 Peter De Rijk
 *
 *	   See the file "README" for information on usage and redistribution
 *	   of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <string.h>
#include <stdlib.h>
#include "tcl.h"
#include "tclInt.h"
#include "odbc.h"
#include "odbc_getinfo.h"

#define STR_LEN 128+1
#define REM_LEN 254+1
#define TABLES_NONSYSTEM 0
#define TABLES_TABLES 'T'
#define TABLES_VIEWS 'V'
#define TABLES_SYSTEM 'S'

static SQLHENV dbi_odbc_env;

int Dbi_odbc_Clone(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	Tcl_Obj *name);

/******************************************************************/

int dbi_odbc_autocommit_state(
	dbi_odbc_Data *dbdata)
{
	if (dbdata->parent != NULL) {
		return dbdata->parent->autocommit;
	} else {
		return dbdata->autocommit;
	}
}

int dbi_odbc_autocommit(
	dbi_odbc_Data *dbdata,
	int state)
{
	if (dbdata->parent != NULL) {
		dbdata->parent->autocommit = state;
	} else {
		dbdata->autocommit = state;
	}
}

int dbi_odbc_trans_state(
	dbi_odbc_Data *dbdata)
{
	if (dbdata->parent != NULL) {
		return dbdata->parent->trans;
	} else {
		return dbdata->trans;
	}
}

int dbi_odbc_trans(
	dbi_odbc_Data *dbdata,
	int state)
{
	if (dbdata->parent != NULL) {
		dbdata->parent->trans = state;
	} else {
		dbdata->trans = state;
	}
}

void dbi_odbc_error(
	Tcl_Interp *interp,
	long erg,
	SQLHDBC hdbc,
	SQLHSTMT hstmt)
{
	char SqlMessage[SQL_MAX_MESSAGE_LENGTH];
	char SqlState[6];
	SDWORD NativeError;
	SWORD Available;
	RETCODE rc;
	switch (erg) {
		case SQL_NO_DATA_FOUND:
			Tcl_AppendResult(interp,"ODBC Error: no data found",NULL);
			return; 
		case SQL_INVALID_HANDLE:
			Tcl_AppendResult(interp,"ODBC Error: invalid handle",NULL);
			return; 
	}
/*	rc = SQLError(dbi_odbc_env, hdbc, hstmt, 
		(UCHAR*) SqlState, &NativeError, (UCHAR*) SqlMessage, 
		SQL_MAX_MESSAGE_LENGTH-1, &Available);*/
	rc = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1,
		(UCHAR*) SqlState, &NativeError, (UCHAR*) SqlMessage, 
		SQL_MAX_MESSAGE_LENGTH-1, &Available);
	if (rc != SQL_ERROR) {
		Tcl_AppendResult(interp, SqlMessage, NULL);
	} else {
		Tcl_AppendResult(interp, "no error message", NULL);
	}
}

int dbi_sql_colname(
	Tcl_Interp *interp,
	SQLHSTMT hstmt,
	int col,
	Tcl_Obj **result
)
{
	RETCODE rc;
	char fbuffer[256];
	char *buffer=fbuffer;
	SWORD reallen;
	rc = SQLColAttributes(hstmt, (UWORD)(col), SQL_COLUMN_NAME, 
		buffer, 256, &reallen, NULL);
	if (rc == SQL_SUCCESS_WITH_INFO) {
		buffer = (void *)Tcl_Alloc(reallen);
		rc = SQLColAttributes(hstmt, (UWORD)(col), SQL_COLUMN_NAME, 
			buffer, reallen, &reallen, NULL);
	}
	if (rc == SQL_ERROR) {
		Tcl_AppendResult(interp,"ODBC Error getting column name: ",NULL);
		dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
		if (buffer != fbuffer) {Tcl_Free(buffer);}
		return TCL_ERROR;
	}
	*result = Tcl_NewStringObj(buffer,reallen);
	if (buffer != fbuffer) {Tcl_Free(buffer);}
	return TCL_OK;
}

int dbi_odbc_Transaction_Rollback(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata)
{
	RETCODE rc;
	if ((dbi_odbc_autocommit_state(dbdata))&&(dbi_odbc_trans_state(dbdata))) {
		rc = SQLEndTran(SQL_HANDLE_DBC,dbdata->hdbc,SQL_ROLLBACK);
		dbi_odbc_trans(dbdata,0);
		if (rc) {
			Tcl_AppendResult(interp,"error rolling back transaction:\n", NULL);
			dbi_odbc_error(interp,rc,dbdata->hdbc,SQL_NULL_HSTMT);
			return TCL_ERROR;
		}
/* fprintf(stdout," ---- rolled back ---- \n");fflush(stdout); */
	}
	return TCL_OK;
}

int dbi_odbc_Transaction_Start(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata)
{
	RETCODE rc;
	if ((dbi_odbc_autocommit_state(dbdata))&&(dbi_odbc_trans_state(dbdata))) {
		rc = SQLEndTran(SQL_HANDLE_DBC,dbdata->hdbc,SQL_COMMIT);
		if (rc) {
			Tcl_AppendResult(interp,"error starting transaction:\n", NULL);
			dbi_odbc_error(interp,rc,dbdata->hdbc,SQL_NULL_HSTMT);
			dbi_odbc_Transaction_Rollback(interp,dbdata);
			return TCL_ERROR;
		}
	}
	dbi_odbc_trans(dbdata,1);
	return TCL_OK;
}

int dbi_odbc_Transaction_Commit(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata)
{
	RETCODE rc;
	if ((dbi_odbc_autocommit_state(dbdata))&&(dbi_odbc_trans_state(dbdata))) {
		rc = SQLEndTran(SQL_HANDLE_DBC,dbdata->hdbc,SQL_COMMIT);
		if (rc) {
			Tcl_AppendResult(interp,"error committing transaction:\n", NULL);
			dbi_odbc_error(interp,rc,dbdata->hdbc,SQL_NULL_HSTMT);
			dbi_odbc_Transaction_Rollback(interp,dbdata);
			return TCL_ERROR;
		}
/* fprintf(stdout," ---- committed ---- \n");fflush(stdout); */
		dbi_odbc_trans(dbdata,0);
	}
	return TCL_OK;
}

int dbi_odbc_Open(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	SQLHSTMT hstmt;
	char buffer[100];
	SQLSMALLINT len=0;
	SQLUINTEGER uint;
	char *ds, *user = "", *pw = "", *string;
	long erg;
	int dslen=0, userlen=0, pwlen=0,stringlen,i;
	if (DB_OPENCONN(dbdata)) {
		Tcl_AppendResult(interp,"dbi object has open connection, close first", NULL);
		return TCL_ERROR;
	}
	if (objc < 3) {
		Tcl_WrongNumArgs(interp,2,objv,"datasource ?options?");
		return TCL_ERROR;
	}
	i = 3;
	while (i < objc) {
		string = Tcl_GetStringFromObj(objv[i],&stringlen);
		if (string[0] != '-') break;
		if ((stringlen==5)&&(strncmp(string,"-user",5)==0)) {
			i++;
			if (i == objc) {
				Tcl_AppendResult(interp,"no value given for option \"-user\"",NULL);
				return TCL_ERROR;
			}
			user = Tcl_GetStringFromObj(objv[i],&userlen);
		} else if ((stringlen==9)&&(strncmp(string,"-password",9)==0)) {
			i++;
			if (i == objc) {
				Tcl_AppendResult(interp,"no value given for option \"-password\"",NULL);
				return TCL_ERROR;
			}
			pw = Tcl_GetStringFromObj(objv[i],&pwlen);
		} else {
			Tcl_AppendResult(interp,"unknown option \"",string,"\", must be one of: -user, -password",NULL);
			return TCL_ERROR;
		}
		i++;
	}
	/* allocate connection handle, set timeout */
	erg = SQLAllocHandle(SQL_HANDLE_DBC, dbi_odbc_env, &dbdata->hdbc); 
	if (erg == SQL_ERROR)	{
		Tcl_AppendResult(interp, "Error AllocHDB",NULL);
		return TCL_ERROR;
	}
	SQLSetConnectAttr(dbdata->hdbc, SQL_ATTR_AUTOCOMMIT , (SQLUINTEGER *)SQL_AUTOCOMMIT_OFF , 0);
	SQLSetConnectAttr(dbdata->hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0);
	ds = Tcl_GetStringFromObj(objv[2],&dslen);
	erg = SQLConnect(dbdata->hdbc, (SQLCHAR*) ds, dslen,(SQLCHAR*) user, userlen,
		(SQLCHAR*) pw, pwlen);
	if (erg == SQL_ERROR)	{
		Tcl_AppendResult(interp,"ODBC Error SQLConnect: ",NULL);
		dbi_odbc_error(interp,erg,dbdata->hdbc,SQL_NULL_HSTMT);
		dbi_odbc_Transaction_Rollback(interp,dbdata);
		return TCL_ERROR;
	}
	dbdata->hasconn = 1;
	erg=SQLAllocHandle(SQL_HANDLE_STMT, dbdata->hdbc, &hstmt);
	if ((erg != SQL_SUCCESS) && (erg != SQL_SUCCESS_WITH_INFO)) {
		Tcl_AppendResult(interp,"ODBC Error allocating statement handle",NULL);
		dbi_odbc_error(interp,erg,SQL_NULL_HDBC,dbdata->hstmt);
		dbi_odbc_Transaction_Rollback(interp,dbdata);
		return TCL_ERROR;
	}
	dbdata->hstmt = hstmt;
	/* get information about the database and driver */
	SQLGetInfo(dbdata->hdbc,SQL_DBMS_NAME,(SQLPOINTER)buffer,(SQLSMALLINT)99,&len);
	dbdata->dbms_name = Tcl_NewStringObj(buffer,(int)len);
	Tcl_IncrRefCount(dbdata->dbms_name);
	SQLGetInfo(dbdata->hdbc,SQL_DBMS_VER,(SQLPOINTER)buffer,(SQLSMALLINT)99,&len);
	if (len > 5) {len = 5;}
	dbdata->dbms_ver = Tcl_NewStringObj(buffer,(int)len);
	Tcl_IncrRefCount(dbdata->dbms_ver);
	SQLGetInfo(dbdata->hdbc,SQL_DYNAMIC_CURSOR_ATTRIBUTES1,&uint,0,NULL);
/*
	if (uint && SQL_CA1_ABSOLUTE) {
		dbdata->supportpos = 1;
	} else {
		dbdata->supportpos = 0;
	}
sometimes they lie!!
*/
	dbdata->supportpos = 0;
	return TCL_OK;
}

int dbi_odbc_Process_statement(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	int cmdlen,
	char *cmdstring,
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	SQLHSTMT hstmt = dbdata->hstmt;
	SQLSMALLINT NumParams;
	Tcl_Obj *tempstring;
	char *string, *nullstring;
	long erg;
	int error,len,nulllen;
	if (nullvalue == NULL) {
		nullstring = NULL; nulllen = 0;
	} else {
		nullstring = Tcl_GetStringFromObj(nullvalue,&nulllen);
	}
	if (!DB_OPENCONN(dbdata)) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		return TCL_ERROR;
	}
	if (cmdlen == 0) {return TCL_OK;}
	error = dbi_odbc_clearresult(dbdata->hstmt,&(dbdata->result));
	if (error) {return error;}
	erg = SQLPrepare(hstmt,cmdstring,cmdlen);
	if (erg != SQL_SUCCESS) {
		Tcl_AppendResult(interp,"ODBC Error preparing statement:\n",NULL);
		dbi_odbc_error(interp,erg,SQL_NULL_HDBC,dbdata->hstmt);
		dbi_odbc_Transaction_Rollback(interp,dbdata);
		goto error;
	}
	erg = SQLNumParams(hstmt, &NumParams);
	if (erg != SQL_SUCCESS) {
		Tcl_AppendResult(interp,"ODBC Error getting number of parameters:\n",NULL);
		dbi_odbc_error(interp,erg,SQL_NULL_HDBC,dbdata->hstmt);
		dbi_odbc_Transaction_Rollback(interp,dbdata);
		goto error;
	}
	if ((int)NumParams != objc) {
		Tcl_AppendResult(interp,"wrong number of arguments given to exec", NULL);
		goto error;
	}
	if (NumParams) {
		SQLSMALLINT i,DataType, DecimalDigits, Nullable, type;
		SQLUINTEGER ParamSize;
		ParamBuffer *sqlparam = NULL;
		char *argstring;
		int arglen;
		sqlparam = (ParamBuffer *)Tcl_Alloc(NumParams*sizeof(ParamBuffer));
		for (i = 0; i < NumParams; i++) {
			/* Describe the parameter. */
			SQLDescribeParam(hstmt, i + 1, &DataType, &ParamSize, &DecimalDigits, &Nullable);
			argstring = Tcl_GetStringFromObj(objv[(int)i],&arglen);
			if (Nullable && (nullstring != NULL) &&(arglen == nulllen)&&(strncmp(argstring,nullstring,len) == 0)) {
				type = SQL_C_CHAR;
				sqlparam[i].paramdata = NULL;
				sqlparam[i].arglen = SQL_NULL_DATA;
				/* Bind the memory to the parameter. We only have input parameters. */
				SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, type, DataType, ParamSize,
				 	  DecimalDigits, "", 0, &(sqlparam[i].arglen));
			} else {
				switch (DataType) {
					case SQL_TYPE_TIMESTAMP:
						{
						TIMESTAMP_STRUCT *ts;
						int number;
						ts = (TIMESTAMP_STRUCT *)Tcl_Alloc(sizeof(TIMESTAMP_STRUCT));
						number = sscanf(argstring,"%4d-%2d-%2d %2d:%2d:%2d.%03d",
							(int *)&(ts->year),(int *)&(ts->month),(int *)&(ts->day),
							(int *)&(ts->hour),(int *)&(ts->minute),(int *)&(ts->second),
							(int *)&(ts->fraction));
						if (number == 7) {
							ts->fraction *= 1000000;
							type = SQL_C_TYPE_TIMESTAMP;
							sqlparam[i].paramdata = (char *)ts;
							sqlparam[i].arglen = arglen;
							type = SQL_C_CHAR;
							sqlparam[i].paramdata = NULL;
							sqlparam[i].arglen = arglen;
							DecimalDigits = 3;
						} else {
							Tcl_Free((char *)ts);
							type = SQL_C_CHAR;
							sqlparam[i].paramdata = NULL;
							sqlparam[i].arglen = arglen;
						}
						}
					default:
						type = SQL_C_CHAR;
						sqlparam[i].paramdata = NULL;
						sqlparam[i].arglen = arglen;
						break;
				}
				/* Bind the memory to the parameter. We only have input parameters. */
				SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, type, DataType, ParamSize,
				 	  DecimalDigits, argstring, arglen, &(sqlparam[i].arglen));
			}
		}
		dbdata->rc = SQLExecute(hstmt);
		if (!SQL_SUCCEEDED(dbdata->rc)) {
			dbi_odbc_error(interp,erg,SQL_NULL_HDBC,dbdata->hstmt);
			dbi_odbc_Transaction_Rollback(interp,dbdata);
		}
		SQLFreeStmt(hstmt,SQL_RESET_PARAMS);
		for (i = 0; i < NumParams; i++) {
			if (sqlparam[i].paramdata != NULL) {Tcl_Free((char *)sqlparam[i].paramdata);}
		}
		Tcl_Free((char *)sqlparam);
		if (!SQL_SUCCEEDED(dbdata->rc)) {
			goto error;
		}
	} else {
		dbdata->rc = SQLExecute(hstmt);
		if (!SQL_SUCCEEDED(dbdata->rc)) {
			dbi_odbc_error(interp,erg,SQL_NULL_HDBC,dbdata->hstmt);
			dbi_odbc_Transaction_Rollback(interp,dbdata);
			goto error;
		}
	}
	return TCL_OK;
	error:
		tempstring = Tcl_NewStringObj(cmdstring,cmdlen);
		Tcl_AppendResult(interp," while executing command: \"",	Tcl_GetStringFromObj(tempstring,NULL), "\"", NULL);
		Tcl_DecrRefCount(tempstring);
		return TCL_ERROR;
}

int dbi_odbc_Find_Prev(
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

int dbi_odbc_SplitSql(
	char *cmdstring,
	int cmdlen,
	int *number,
	int **lens)
{
	char *nextline;
	nextline = strchr(cmdstring,';');
	if (nextline == NULL) {
		*lens = NULL;
		*number = 1;
	} else {
		char *find;
		int *result = NULL;
		int i,blevel,level,start,prevline,pos,len;
		pos = 0;
		result = (int *)Tcl_Alloc((pos+1)*sizeof(int));
		i = 0;
		blevel = 0;
		level = 0;
		while (i <= cmdlen) {
			if ((cmdstring[i] != ' ')&&(cmdstring[i] != '\n')&&(cmdstring[i] != '\t')) break;
			i++;
		}
		start = 0;
		prevline = i;
		while (1) {
			if (cmdstring[i] == '\n') {
				/* fprintf(stdout,"# %d,%d %*.*s\n",blevel,level,i-prevline,i-prevline,cmdstring+prevline);fflush(stdout); */
				if (dbi_odbc_Find_Prev(cmdstring+prevline,i-prevline,"begin")) {
					blevel++;
				} else if (dbi_odbc_Find_Prev(cmdstring+prevline,i-prevline,"BEGIN")) {
					blevel++;
				} else if (dbi_odbc_Find_Prev(cmdstring+prevline,i-prevline,"end")) {
					blevel--;
				} else if (dbi_odbc_Find_Prev(cmdstring+prevline,i-prevline,"END")) {
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
				if (dbi_odbc_Find_Prev(cmdstring+prevline,i-prevline,"end")) {
					blevel--;
				} else if (dbi_odbc_Find_Prev(cmdstring+prevline,i-prevline,"END")) {
					blevel--;
				}
				find = strstr(cmdstring+prevline,"end");
				if (((level != 0)||(blevel != 0))&&(i != cmdlen)) {
					i++;
					continue;
				}
				/* fprintf(stdout,"** %*.*s\n",i-start,i-start,cmdstring+start);fflush(stdout); */
				result = (int *)Tcl_Realloc((char *)result,(pos+1)*sizeof(int));
				result[pos++] = i-start;
				if (i == cmdlen) break;
				start = i+1;
				while (i < cmdlen) {
					i++;
					if (i == cmdlen) break;
					if ((cmdstring[i] != ' ')&&(cmdstring[i] != '\n')&&(cmdstring[i] != '\t')) break;
				}
				if (i == cmdlen) break;
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
		if (pos == 1) {
			if (result == NULL) {Tcl_Free((char *)result);}
			*number = 1;
		} else {
			*lens = result;
			*number = pos;
		}
	}
	return TCL_OK;
}

int dbi_odbc_Exec(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	Tcl_Obj *cmd,
	int usefetch,
	int flat,
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	SQLHSTMT hstmt = dbdata->hstmt;
	char *command;
	int *lens = NULL;
	int error, commandlen,number,i,showcmd = 0;
	if (!DB_OPENCONN(dbdata)) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		return TCL_ERROR;
	}
	command = Tcl_GetStringFromObj(cmd,&commandlen);
	if (commandlen == 0) {return TCL_OK;}

/*
fprintf(stdout,"************************************\n");fflush(stdout);
fprintf(stdout,"%s\n",command);fflush(stdout);
fprintf(stdout," ---- trans: %d autocommit: %d\n",dbi_odbc_trans_state(dbdata),dbi_odbc_autocommit_state(dbdata));fflush(stdout);
*/
	error = dbi_odbc_clearresult(hstmt,&(dbdata->result));
	if (error) {return error;}
	if (dbi_odbc_Transaction_Start(interp,dbdata) != TCL_OK) {goto error;}
	error = dbi_odbc_SplitSql(command,commandlen,&number,&lens);
	if (error) {goto error;}
	if (number == 1) {
		error = dbi_odbc_Process_statement(interp,dbdata,commandlen,command,nullvalue,objc,objv);
		if (error) {/*dbi_odbc_autocommit(dbdata,1);*/goto error;}
	} else {
		if (objc != 0) {
			Tcl_AppendResult(interp,"multiple commands cannot take arguments\n", NULL);
			goto error;
		}
		for(i = 0 ; i < number ; i++) {
			/* fprintf(stdout,"%d: %*.*s\n",lens[i],lens[i],lens[i],command);fflush(stdout); */
			error = dbi_odbc_Process_statement(interp,dbdata,lens[i],command,nullvalue,0,NULL);
			if (error) {/*dbi_odbc_autocommit(dbdata,1);*/goto error;}
			command += lens[i]+1;
		}
		Tcl_Free((char *)lens); lens = NULL;
	}
	switch (dbdata->rc) {
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO: 
			error = dbi_odbc_bindresult(interp,hstmt,&(dbdata->result));
			if (error) {
				dbi_odbc_Transaction_Rollback(interp,dbdata);
				return error;
			}
			Tcl_ResetResult(interp);
			if (dbdata->result.nfields != 0) {
				if (!usefetch) {
					if (nullvalue == NULL) {nullvalue = 	dbdata->defnullvalue;}
					if (flat) {
						error = dbi_odbc_ToResult_flat(interp,hstmt,&(dbdata->result),nullvalue);
					} else {
						error = dbi_odbc_ToResult(interp,hstmt,&(dbdata->result),nullvalue);
					}
					if (error) {showcmd = 1;goto error;}
					if (dbi_odbc_Transaction_Commit(interp,dbdata) != TCL_OK) {
						goto error;
					}
				}
			} else {
				if (dbi_odbc_Transaction_Commit(interp,dbdata) != TCL_OK) {
					goto error;
				}
			}
			break;
		default:
			dbi_odbc_error(interp,dbdata->rc,SQL_NULL_HDBC,hstmt);
			dbi_odbc_Transaction_Rollback(interp,dbdata);
			showcmd = 1;
			goto error;
	}
	if (!usefetch) {
		error = dbi_odbc_clearresult(hstmt,&(dbdata->result));
		if (error) {return error;}
	}
/*	dbdata->tuple = -1;*/
	return TCL_OK;
	error:
		if (lens != NULL) {Tcl_Free((char *)lens);}
		if (showcmd) {
			Tcl_AppendResult(interp," while executing command: \"",	Tcl_GetStringFromObj(cmd,NULL), "\"", NULL);
		}
		dbi_odbc_Transaction_Rollback(interp,dbdata);
		return TCL_ERROR;
}

int dbi_odbc_Fetch(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	SQLHSTMT hstmt = dbdata->hstmt;
	odbc_Result *dbresult = &(dbdata->result);
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
	int i, ituple = -1, ifield = -1;
	Tcl_Obj *line = NULL, *element = NULL;
	RETCODE rc;
	int error;
	int nfields = dbresult->nfields;
	if (dbresult->buffer == NULL) {
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
				ituple = dbresult->tuple;
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
			if (dbresult->ntuples == -1) {
				Tcl_AppendResult(interp,"lines not supported by odbc driver",NULL);
				return TCL_ERROR;
			}
			Tcl_SetObjResult(interp,Tcl_NewIntObj(dbresult->ntuples));
			return TCL_OK;
		case Fields:
			line = Tcl_NewListObj(0,NULL);
			for (i=0; i < nfields; ++i) {
				error = dbi_sql_colname(interp,hstmt,i+1,&element);
				if (error) {Tcl_DecrRefCount(line);return error;}
				error = Tcl_ListObjAppendElement(interp,line, element);
				if (error) {Tcl_DecrRefCount(line);Tcl_DecrRefCount(element);return TCL_ERROR;}
			}
			Tcl_SetObjResult(interp, line);
			return TCL_OK;
		case Clear:
			error = dbi_odbc_clearresult(hstmt,dbresult);
			return error;
		case Pos:
			Tcl_SetObjResult(interp,Tcl_NewIntObj(dbresult->tuple));
			return TCL_OK;
	}
	/*
	 * fetch data
	 * ----------
	 */
	if (ituple == -1) {
		ituple = dbresult->tuple+1;
	}
	if (dbresult->tuple == -1) {
		/* get to the first line in the result, if this is the first call to fetch */
		rc = SQLFetch(hstmt);
		switch (rc) {
			case SQL_NO_DATA_FOUND:
				dbresult->ntuples = dbresult->tuple;
				goto out_of_position;
			case SQL_ERROR:
			case SQL_INVALID_HANDLE:
				Tcl_AppendResult(interp,"ODBC Error during fetch: ",NULL);
				dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
				dbi_odbc_Transaction_Rollback(interp,dbdata);
				return TCL_ERROR;
		}
		dbresult->tuple++;		
	}
	if ((dbresult->ntuples != -1)&&(ituple > dbresult->ntuples)) {
		goto out_of_position;
	}
	if (ifield >= nfields) {
		Tcl_Obj *buffer;
		buffer = Tcl_NewIntObj(ifield);
		Tcl_AppendResult(interp, "field ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
		Tcl_DecrRefCount(buffer);
		return TCL_ERROR;
	}
	/* move to the requested line */
	if (ituple != dbresult->tuple) {
		if (dbdata->supportpos) {
			rc = SQLFetchScroll(hstmt,SQL_FETCH_ABSOLUTE,ituple+1);
			switch (rc) {
				case SQL_NO_DATA_FOUND:
					goto out_of_position;
				case SQL_ERROR:
				case SQL_INVALID_HANDLE:
					Tcl_AppendResult(interp,"ODBC Error during fetch: ",NULL);
					dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
					dbi_odbc_Transaction_Rollback(interp,dbdata);
					return TCL_ERROR;
			}
			dbresult->tuple = ituple;
		} else {
			if (ituple < dbresult->tuple) {
				Tcl_AppendResult(interp,"odbc driver error: backwards positioning for fetch not supported",NULL);
				return TCL_ERROR;
			}
			while (dbresult->tuple < ituple) {
				rc = SQLFetch(hstmt);
				switch (rc) {
					case SQL_NO_DATA_FOUND:
						dbresult->ntuples = dbresult->tuple;
						goto out_of_position;
					case SQL_ERROR:
					case SQL_INVALID_HANDLE:
						Tcl_AppendResult(interp,"ODBC Error during fetch: ",NULL);
						dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
						dbi_odbc_Transaction_Rollback(interp,dbdata);
						return TCL_ERROR;
				}
				dbresult->tuple++;		
			}
		}
	}
	switch (fetch_option) {
		case Data:
			if (nullvalue == NULL) {
				nullvalue = dbdata->defnullvalue;
			}
			if (ifield == -1) {
				error = dbi_odbc_GetRow(interp,hstmt,dbresult,nullvalue,&line);
				if (error) {return TCL_ERROR;}
				Tcl_SetObjResult(interp, line);
			} else {
				error = dbi_odbc_GetOne(interp,hstmt,dbresult,ifield,&element);
				if (error) {return TCL_ERROR;}
				if (element == NULL) {
					element = nullvalue;
				}
				Tcl_SetObjResult(interp, element);
			}
			break;
		case Isnull:
			if (dbresult->buffer[ifield].cbValue == SQL_NULL_DATA) {
				Tcl_SetObjResult(interp,Tcl_NewIntObj(1));
			} else {
				Tcl_SetObjResult(interp,Tcl_NewIntObj(0));
			}
			return TCL_OK;
	}
	return TCL_OK;
	out_of_position:
		{
		Tcl_Obj *buffer;
		buffer = Tcl_NewIntObj(dbresult->tuple+1);
		Tcl_AppendResult(interp, "line ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
		Tcl_DecrRefCount(buffer);
		return TCL_ERROR;
		}
}

int dbi_odbc_Close(
	dbi_odbc_Data *dbdata)
{
	if (!DB_OPENCONN(dbdata)) return TCL_OK;
	dbi_odbc_clearresult(dbdata->hstmt,&(dbdata->result));
	SQLFreeHandle(SQL_HANDLE_STMT,dbdata->hstmt);
	if (dbi_odbc_trans_state(dbdata)) {
		SQLEndTran(SQL_HANDLE_DBC,dbdata->hdbc,SQL_ROLLBACK);
		dbi_odbc_trans(dbdata,0);
	}
	while (dbdata->clonesnum) {
		Tcl_DeleteCommandFromToken(dbdata->interp,dbdata->clones[0]);
	}
	if (dbdata->clones != NULL) {
		Tcl_Free((char *)dbdata->clones);
		dbdata->clones = NULL;
	}
	if (dbdata->parent == NULL) {
		SQLDisconnect(dbdata->hdbc);
		SQLFreeHandle(SQL_HANDLE_DBC,dbdata->hdbc);
		if (dbdata->dbms_name != NULL) {
			Tcl_DecrRefCount(dbdata->dbms_name);
			dbdata->dbms_name = NULL;
		}
		if (dbdata->dbms_ver != NULL) {
			Tcl_DecrRefCount(dbdata->dbms_ver);
			dbdata->dbms_ver = NULL;
		}
		dbdata->hasconn = 0;
	} else {
		dbi_odbc_Data *parent = dbdata->parent;
		int i;
		for (i = 0 ; i < parent->clonesnum; i++) {
			if (parent->clones[i] == dbdata->token) break;
		}
		i++;
		for (; i < parent->clonesnum; i++) {
			parent->clones[i-1] = parent->clones[i];
		}
		parent->clonesnum--;
	}
	return TCL_OK;
}

int dbi_odbc_Tables(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	int type)
{
	SQLHSTMT hstmt;
	RETCODE rc;
	SQLCHAR buffer[STR_LEN],typebuffer[STR_LEN];
	SQLINTEGER size,typesize;
	rc=SQLAllocHandle(SQL_HANDLE_STMT, dbdata->hdbc, &hstmt);
	if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) {
		Tcl_AppendResult(interp,"ODBC Error allocating statement handle",NULL);
		dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
		dbi_odbc_Transaction_Rollback(interp,dbdata);
		return TCL_ERROR;
	}
	rc = SQLTables(hstmt, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS);
	switch (rc) {
		case SQL_ERROR:
		case SQL_INVALID_HANDLE:
			Tcl_AppendResult(interp,"ODBC Error getting tables: ",NULL);
			dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
			SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
			return TCL_ERROR;
	}
	SQLBindCol(hstmt, 3, SQL_C_CHAR, buffer, STR_LEN, &size);
	SQLBindCol(hstmt, 4, SQL_C_CHAR, typebuffer, STR_LEN, &typesize);
	while(1) {
		rc = SQLFetch(hstmt);
/*
fprintf(stdout,"%s: %c, type: %c\n",buffer,typebuffer[0],type);fflush(stdout);
fprintf(stdout,"typebuffer: %s\n",typebuffer);fflush(stdout);
*/
		if (rc == SQL_NO_DATA_FOUND) {
			break;
		} else if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO){
			if ((typebuffer[0] == type) || ((type == TABLES_NONSYSTEM) && typebuffer[0] != 'S')) {
				Tcl_AppendElement(interp,buffer);
			}
		} else {
			Tcl_AppendResult(interp,"ODBC Error getting tables: ",NULL);
			dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
			SQLCloseCursor(hstmt);
			SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
			return TCL_ERROR;
		}
	}
	SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
	return TCL_OK;
}

int dbi_odbc_Fields(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	Tcl_Obj *table)
{
	SQLHSTMT hstmt;
	RETCODE rc;
	SQLCHAR buffer[STR_LEN];
	SQLINTEGER size;
	char *string;
	int stringlen,i;
	string = Tcl_GetStringFromObj(table,&stringlen);
	rc=SQLAllocHandle(SQL_HANDLE_STMT, dbdata->hdbc, &hstmt);
	if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) {
		Tcl_AppendResult(interp,"ODBC Error allocating statement handle",NULL);
		dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
		return TCL_ERROR;
	}
	rc = SQLColumns(hstmt, NULL, SQL_NTS, NULL, SQL_NTS, string, stringlen, NULL, SQL_NTS);
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
		Tcl_AppendResult(interp,"ODBC Error getting fields: ",NULL);
		dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
		SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
		return TCL_ERROR;
	}
	SQLBindCol(hstmt, 4, SQL_C_CHAR, buffer, STR_LEN, &size);
	i = 0;
	while(1) {
		rc = SQLFetch(hstmt);
		if (rc == SQL_NO_DATA_FOUND) {
			break;
		} else if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO){
			Tcl_AppendElement(interp,buffer);
		} else {
			Tcl_AppendResult(interp,"ODBC Error getting fields: ",NULL);
			dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
			SQLCloseCursor(hstmt);
			SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
			return TCL_ERROR;
		}
		i++;
	}
	SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
	if (i == 0) {
		Tcl_AppendResult(interp,"table \"",string,"\" does not exist",NULL);
		return TCL_ERROR;
	}
	return TCL_OK;
}

int dbi_odbc_Catalog(
	Tcl_Interp *interp,
	SQLHDBC hdbc,
	int type,
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	SQLHSTMT hstmt;
	RETCODE rc;
	odbc_Result dbresult;
	char *strbuffer[8];
	int lenbuffer[8];
	int i,error;
	if (objc > 8) {
		Tcl_AppendResult(interp,"objc > 8",NULL);return TCL_ERROR;
	}
	for (i = 0 ; i < objc ; i++) {
		strbuffer[i] = Tcl_GetStringFromObj(objv[i],lenbuffer+i);
		if (lenbuffer == 0) {strbuffer[i] = NULL; lenbuffer[i] = SQL_NTS;}
	}
	rc=SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) {
		Tcl_AppendResult(interp,"ODBC Error allocating statement handle",NULL);
		dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
		return TCL_ERROR;
	}
	switch (type) {
		case CATALOG_SQLColumns:
			rc = SQLColumns(hstmt, strbuffer[0], lenbuffer[0], strbuffer[1], lenbuffer[1],
				strbuffer[2], lenbuffer[2], strbuffer[3], lenbuffer[3]);
			break;
		case CATALOG_SQLPrimaryKeys:
			rc = SQLPrimaryKeys(hstmt, strbuffer[0], lenbuffer[0], strbuffer[1], lenbuffer[1],
				strbuffer[2], lenbuffer[2]);
			break;
		case CATALOG_SQLTablePrivileges:
			rc = SQLTablePrivileges(hstmt, strbuffer[0], lenbuffer[0], strbuffer[1], lenbuffer[1],
				strbuffer[2], lenbuffer[2]);
			break;
		case CATALOG_SQLColumnPrivileges:
			rc = SQLColumnPrivileges(hstmt, strbuffer[0], lenbuffer[0], strbuffer[1], lenbuffer[1],
				strbuffer[2], lenbuffer[2], strbuffer[3], lenbuffer[3]);
			break;
		case CATALOG_SQLForeignKeys:
			rc = SQLForeignKeys(hstmt, strbuffer[0], lenbuffer[0], strbuffer[1], lenbuffer[1],
				strbuffer[2], lenbuffer[2], strbuffer[3], lenbuffer[3],
				strbuffer[4], lenbuffer[4], strbuffer[5], lenbuffer[5]);
			break;
		case CATALOG_SQLTables:
			rc = SQLTables(hstmt, strbuffer[0], lenbuffer[0], strbuffer[1], lenbuffer[1],
				strbuffer[2], lenbuffer[2], strbuffer[3], lenbuffer[3]);
			break;
		case CATALOG_SQLSpecialColumns:
			{
			char *string;
			int bool,scope,len,type;
			string = strbuffer[0];
			len = lenbuffer[0];
			if ((len == 10)&&(strncmp(string,"best_rowid",len)==0)) {
				type = SQL_BEST_ROWID;
			} else if ((len == 6)&&(strncmp(string,"rowver",len)==0)) {
				type = SQL_ROWVER;
			} else {
				Tcl_AppendResult(interp,"wrong value for type: must be one of:	best_rowid or rowver",NULL);
				return TCL_ERROR;
			}
			string = strbuffer[4];
			len = lenbuffer[4];
			if ((len == 6)&&(strncmp(string,"currow",len)==0)) {
				scope = SQL_SCOPE_CURROW;
			} else if ((len == 11)&&(strncmp(string,"transaction",len)==0)) {
				scope = SQL_SCOPE_TRANSACTION;
			} else if ((len == 7)&&(strncmp(string,"session",len)==0)) {
				scope = SQL_SCOPE_SESSION;
			} else {
				Tcl_AppendResult(interp,"wrong value for scope: must be one of:	currow, transaction or session",NULL);
				return TCL_ERROR;
			}
			error = Tcl_GetBooleanFromObj(interp,objv[5],&bool);
			if (error) {return error;}
			if (bool) {bool = SQL_NULLABLE;} else {bool = SQL_NO_NULLS;}
			rc = SQLSpecialColumns(hstmt, type, strbuffer[1], lenbuffer[1], strbuffer[2], lenbuffer[2],
				strbuffer[3], lenbuffer[3], scope, bool);
			}
			break;
		case CATALOG_SQLStatistics:
			{
			int unique,quick;
			error = Tcl_GetBooleanFromObj(interp,objv[3],&unique);
			if (error) {return error;}
			if (unique) {unique = SQL_INDEX_UNIQUE;} else {unique = SQL_INDEX_ALL;}
			error = Tcl_GetBooleanFromObj(interp,objv[4],&quick);
			if (error) {return error;}
			if (quick) {quick = SQL_QUICK;} else {quick = SQL_ENSURE;}
			rc = SQLSpecialColumns(hstmt, type, 
				strbuffer[0], lenbuffer[0], strbuffer[1], lenbuffer[1],
				strbuffer[2], lenbuffer[2], unique, quick);
			}
			break;
		case CATALOG_SQLProcedures:
			rc = SQLProcedures(hstmt, strbuffer[0], lenbuffer[0], strbuffer[1], lenbuffer[1],
				strbuffer[2], lenbuffer[2]);
			break;
		case CATALOG_SQLProcedureColumns:
			rc = SQLProcedureColumns(hstmt, 
				strbuffer[0], lenbuffer[0], strbuffer[1], lenbuffer[1],
				strbuffer[2], lenbuffer[2], strbuffer[3], lenbuffer[3]);
			break;
	}
	if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
		Tcl_AppendResult(interp,"ODBC Error getting catalog info: ",NULL);
		dbi_odbc_error(interp,rc,hdbc,hstmt);
		SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
		return TCL_ERROR;
	}
	dbi_odbc_initresult(&dbresult);
	error = dbi_odbc_bindresult(interp,hstmt,&dbresult);
	if (error) {goto error;}
	error = dbi_odbc_ToResult(interp,hstmt,&dbresult,nullvalue);
	if (error) {goto error;}
	error = dbi_odbc_clearresult(hstmt,&dbresult);
	if (error) {goto error;}
	SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
	return TCL_OK;
	error:
		SQLFreeHandle(SQL_HANDLE_STMT,hstmt);
		return TCL_ERROR;
}

int dbi_odbc_SQLGetInfo_options(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata)
{
	Getinfo_cor *infocor;
	int j;
	for (j = 0 ; j < getinfo_size ; j++) {
		if (getinfo_cor[j] == NULL) continue;
		infocor = getinfo_cor[j];
		while (infocor->name != NULL) {
			Tcl_AppendResult(interp,infocor->name," ",NULL);
			infocor++;
		}
	}
	return TCL_OK;
}

static char *getinfobuffer;
static int getinfobuffersize = 0;

int dbi_odbc_SQLGetInfo(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	Tcl_Obj *infotypeObj)
{
	RETCODE rc;
	SQLSMALLINT len=0;
	Getinfo_cor *infocor;
	Getinfo_values *value;
	char *string;
	int stringlen;
	string = Tcl_GetStringFromObj(infotypeObj,&stringlen);
	if ((stringlen == 7) && (strncmp(string,"options",stringlen) == 0)) {
		dbi_odbc_SQLGetInfo_options(interp,dbdata);
		return TCL_OK;
	}
	if (stringlen > getinfo_size) {
		Tcl_AppendResult(interp,"Option \"",string,"\" not supported by sqlgetinfo, must be one of: ",NULL);
		dbi_odbc_SQLGetInfo_options(interp,dbdata);
		return TCL_ERROR;
	}
	infocor = getinfo_cor[stringlen];
	while (infocor != NULL) {
		if (strncmp(string,infocor->name,stringlen) == 0) break;
		infocor++;
	}
	if (infocor == NULL) {
		Tcl_AppendResult(interp,"Option \"",string,"\" not supported by sqlgetinfo, must be one of: ",NULL);
		dbi_odbc_SQLGetInfo_options(interp,dbdata);
		return TCL_ERROR;
	}
	rc = SQLGetInfo(dbdata->hdbc,infocor->code,(SQLPOINTER)getinfobuffer,(SQLSMALLINT)getinfobuffersize,&len);
	if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) {goto error;}
	if (len > getinfobuffersize) {
		getinfobuffer = Tcl_Realloc(getinfobuffer,len+1);
		getinfobuffersize = len+1;
		rc = SQLGetInfo(dbdata->hdbc,infocor->code,(SQLPOINTER)getinfobuffer,(SQLSMALLINT)getinfobuffersize,&len);
		if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) {goto error;}
	}
	switch (infocor->type) {
		case STRING:
			Tcl_SetObjResult(interp,Tcl_NewStringObj(getinfobuffer,len));
			break;
		case BITMASK:
			{
				SQLINTEGER *resultPtr;
				resultPtr = (SQLINTEGER *)getinfobuffer;
				value = infocor->values;
				while (value->name != NULL) {
					if (*resultPtr && (SQLINTEGER)value->code) {
						Tcl_AppendElement(interp,value->name);
					}
					value++;
				}
			}
			break;
		case UBITMASK:
			{
				SQLUINTEGER *resultPtr;
				resultPtr = (SQLUINTEGER *)getinfobuffer;
				value = infocor->values;
				while (value->name != NULL) {
					if (*resultPtr &&  (SQLUINTEGER)value->code) {
						Tcl_AppendElement(interp,value->name);
					}
					value++;
				}
			}
			break;
		case VALUE:
			{
				SQLUSMALLINT *resultPtr;
				resultPtr = (SQLUSMALLINT *)getinfobuffer;
				if (infocor->values == NULL) {
					Tcl_SetObjResult(interp,Tcl_NewIntObj((int)*resultPtr));
				} else {
					value = infocor->values;
					while (value->name != NULL) {
						if (*resultPtr == (SQLUSMALLINT)value->code) {
							Tcl_AppendElement(interp,value->name);
							return TCL_OK;
						}
						value++;
					}
					Tcl_SetObjResult(interp,Tcl_NewIntObj((int)*resultPtr));
				}
			}
			break;
		case ENUM:
		case VALUE32:
			{
				SQLUINTEGER *resultPtr;
				resultPtr = (SQLUINTEGER *)getinfobuffer;
				if (infocor->values == NULL) {
					Tcl_SetObjResult(interp,Tcl_NewIntObj((int)*resultPtr));
				} else {
					value = infocor->values;
					while (value->name != NULL) {
						if (*resultPtr == (SQLUINTEGER)value->code) {
							Tcl_AppendElement(interp,value->name);
							return TCL_OK;
						}
						value++;
					}
					Tcl_SetObjResult(interp,Tcl_NewIntObj((int)*resultPtr));
				}
			}
			break;
	}
	return TCL_OK;
	error:
		{
		SQLCHAR SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
		SQLINTEGER NativeError;
		SQLSMALLINT   i, MsgLen;
		SQLRETURN     rc;
		Tcl_AppendResult(interp,"ODBC Error in SQLGetinfo:\n",NULL);
		i = 1;
		while ((rc = SQLGetDiagRec(SQL_HANDLE_DBC, dbdata->hdbc, i, SqlState, &NativeError, Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA) {
			Tcl_AppendResult(interp,Msg,";",NULL);
			i++;   
		}
		}
		return TCL_ERROR;
}

int dbi_odbc_Supports(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	Tcl_Obj *keyword)
{
	static char *keywords[] = {
		"backfetch","transactions","sharedtransactions",
		(char *) NULL};
	enum keywordsIdx {
		Backfetch,Transactions,Sharedtransactions
	};
	int error,index;
	if (keyword == NULL) {
		char **keyword = keywords;
		index = 0;
		while (1) {
			if (*keyword == NULL) break;
			switch(index) {
				case Backfetch:
					if (dbdata->supportpos) {
						Tcl_AppendElement(interp,*keyword);
					}
					break;
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
				case Backfetch:
					Tcl_SetObjResult(interp,Tcl_NewIntObj(dbdata->supportpos));
					break;
				default:
					Tcl_SetObjResult(interp,Tcl_NewIntObj(1));
			}
		} else {
			Tcl_SetObjResult(interp,Tcl_NewIntObj(0));
		}
	}
	return TCL_OK;
}

int dbi_odbc_Info(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	int objc,
	Tcl_Obj **objv)
{
	Tcl_Obj *cmd;
	int error,i;
	cmd = Tcl_NewStringObj("::dbi::odbc::info ",-1);
	Tcl_IncrRefCount(cmd);
	Tcl_GetCommandFullName(interp, dbdata->token, cmd);
	for (i = 0 ; i < objc ; i++) {
		error = Tcl_ListObjAppendElement(interp,cmd,objv[i]);
		if (error) {Tcl_DecrRefCount(cmd);return error;}
	}
	error = Tcl_EvalObj(interp,cmd);
	Tcl_DecrRefCount(cmd);
	return error;
}


int dbi_odbc_Interface(
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	int i;
	static char *interfaces[] = {
		"dbi", DBI_VERSION,
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
		char *interface_name;
		int len;
		interface_name = Tcl_GetStringFromObj(objv[2],&len);
		i = 0;
		while (interfaces[i] != NULL) {
			if ((strlen(interfaces[i]) == len) && (strncmp(interfaces[i],interface_name,len) == 0)) {
				Tcl_AppendResult(interp,interfaces[i+1],NULL);
				return TCL_OK;
			}
			i+=2;
		}
		Tcl_AppendResult(interp, Tcl_GetStringFromObj(objv[0],NULL), 
			" does not support interface ", interface_name, NULL);
		return TCL_ERROR;
	}
}

int dbi_odbc_Serial(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
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
	if (dbdata->dbms_name == NULL) {
		Tcl_AppendResult(interp,"dbms_name was not found, cannot use serial commands",NULL);
		return TCL_ERROR;
	}
	error = Tcl_GetIndexFromObj(interp, subcmd, subCmds, "suboption", 0, (int *) &index);
	if (error != TCL_OK) {
		return error;
	}
	cmd = Tcl_NewStringObj("::dbi::odbc::serial_",-1);
	Tcl_IncrRefCount(cmd);
	Tcl_AppendObjToObj(cmd,dbdata->dbms_name);
    switch (index) {
	    case Add:
			Tcl_AppendStringsToObj(cmd,"_add",NULL);
			break;
		case Delete:
			Tcl_AppendStringsToObj(cmd,"_delete",NULL);
			current = NULL;
			break;
		case Set:
			Tcl_AppendStringsToObj(cmd,"_set",NULL);
			break;
		case Next:
			Tcl_AppendStringsToObj(cmd,"_next",NULL);
			break;
	}
	dbcmd = Tcl_NewObj();
	Tcl_GetCommandFullName(interp, dbdata->token, dbcmd);
	error = Tcl_ListObjAppendElement(interp,cmd,dbcmd);
	if (error) {Tcl_DecrRefCount(cmd);Tcl_DecrRefCount(dbcmd);return error;}
	error = Tcl_ListObjAppendElement(interp,cmd,dbdata->dbms_ver);
	if (error) {Tcl_DecrRefCount(cmd);return error;}
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

int Dbi_odbc_DbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)clientdata;
	int error=TCL_OK,i,index;
	static char *subCmds[] = {
		"interface","open", "exec", "fetch", "close",
		"info", "tables","fields", "tableinfo",
		"begin", "commit", "rollback",
		"destroy", "serial","supports",
		"clone","clones","parent",
		"sqlcolumns","sqlcolumnprivileges","sqltables","sqltableprivileges",
		"sqlprimarykeys","sqlforeignkeys",	"sqlspecialcolumns","sqlstatistics",
		"sqlprocedures","sqlprocedurecolumns","sqlgetinfo",
		(char *) NULL};
	enum ISubCmdIdx {
		Interface, Open, Exec, Fetch, Close,
		Info, Tables, Fields, Tableinfo,
		Begin, Commit, Rollback,
		Destroy, Serial, Supports,
		Clone, Clones, Parent,
		SQLColumns, SQLColumnPrivileges, SQLTables, SQLTablePrivileges,
		SQLPrimaryKeys, SQLForeignKeys,	SQLSpecialColumns, SQLStatistics,
		SQLProcedures,SQLProcedureColumns,SQLGetinfo
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
		return dbi_odbc_Interface(interp,objc,objv);
	case Open:
		if (dbdata->parent != NULL) {
			Tcl_AppendResult(interp,"clone may not use open",NULL);
			return TCL_ERROR;
		}
		return dbi_odbc_Open(interp,dbdata,objc,objv);
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
				Tcl_GetCommandFullName(interp, dbdata->clones[i], dbcmd);
				if (strncmp(Tcl_GetStringFromObj(dbcmd,NULL),"::dbi::odbc::priv_",23) == 0) continue;
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
	if (!DB_OPENCONN(dbdata)) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		return TCL_ERROR;
	}
	switch (index) {
	case Exec:
		{
	    static char *switches[] = {"-usefetch", "-nullvalue", "-flat","-cache",(char *) NULL};
	    enum switchesIdx {Usefetch, Nullvalue, Flat, Cache};
		Tcl_Obj *nullvalue = NULL;
		char *string;
		int usefetch = 0, flat = 0;
		int stringlen;
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
					usefetch = 1;
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
					flat = 1;
					break;
			}
			i++;
		}
		return dbi_odbc_Exec(interp,dbdata,objv[i],usefetch,flat,nullvalue,objc-i-1,objv+i+1);
		}
	case Fetch:
		return dbi_odbc_Fetch(interp,dbdata, objc, objv);
	case Info:
		return dbi_odbc_Info(interp,dbdata,objc-2,objv+2);
	case Tables:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_odbc_Tables(interp,dbdata,TABLES_NONSYSTEM);
	case Fields:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "fields tablename");
			return TCL_ERROR;
		}
		return dbi_odbc_Fields(interp,dbdata,objv[2]);
	case Close:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->parent == NULL) {
			return dbi_odbc_Close(dbdata);
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
		error = dbi_odbc_Transaction_Start(interp,dbdata);
		if (error) {return error;}
		dbi_odbc_autocommit(dbdata,0);
		return TCL_OK;
	case Commit:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		dbi_odbc_autocommit(dbdata,1);
		error = dbi_odbc_Transaction_Commit(interp,dbdata);
		if (error) {return error;}
		return TCL_OK;
	case Rollback:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		dbi_odbc_autocommit(dbdata,1);
		error = dbi_odbc_Transaction_Rollback(interp,dbdata);
		if (error) {return error;}
		return TCL_OK;
	case Serial:
		if (objc < 5) {
			Tcl_WrongNumArgs(interp, 2, objv, "option table field ?value?");
			return TCL_ERROR;
		}
		if (objc == 6) {
			return dbi_odbc_Serial(interp,dbdata,objv[2],objv[3],objv[4],objv[5]);
		} else {
			return dbi_odbc_Serial(interp,dbdata,objv[2],objv[3],objv[4],NULL);
		}
	case Clone:
		if ((objc != 2) && (objc != 3)) {
			Tcl_WrongNumArgs(interp, 2, objv, "?name?");
			return TCL_ERROR;
		}
		if (objc == 3) {
			return Dbi_odbc_Clone(interp,dbdata,objv[2]);
		} else {
			return Dbi_odbc_Clone(interp,dbdata,NULL);
		}
	case SQLColumns:
		if (objc != 6) {
			Tcl_WrongNumArgs(interp, 2, objv, "catalog schema table field");
			return TCL_ERROR;
		}
		error = dbi_odbc_Catalog(interp,dbdata->hdbc,CATALOG_SQLColumns,dbdata->defnullvalue,objc-2,objv+2);
		return TCL_OK;
	case SQLPrimaryKeys:
		if (objc != 5) {
			Tcl_WrongNumArgs(interp, 2, objv, "catalog schema table");
			return TCL_ERROR;
		}
		error = dbi_odbc_Catalog(interp,dbdata->hdbc,CATALOG_SQLPrimaryKeys,dbdata->defnullvalue,objc-2,objv+2);
		return TCL_OK;
	case SQLColumnPrivileges:
		if (objc != 6) {
			Tcl_WrongNumArgs(interp, 2, objv, "catalog schema table field");
			return TCL_ERROR;
		}
		error = dbi_odbc_Catalog(interp,dbdata->hdbc,CATALOG_SQLColumnPrivileges,dbdata->defnullvalue,objc-2,objv+2);
		return TCL_OK;
	case SQLTablePrivileges:
		if (objc != 5) {
			Tcl_WrongNumArgs(interp, 2, objv, "catalog schema table");
			return TCL_ERROR;
		}
		error = dbi_odbc_Catalog(interp,dbdata->hdbc,CATALOG_SQLTablePrivileges,dbdata->defnullvalue,objc-2,objv+2);
		return TCL_OK;
	case SQLForeignKeys:
		if (objc != 8) {
			Tcl_WrongNumArgs(interp, 2, objv, "pkcatalog pkschema pktable fkcatalog fkschema fktable");
			return TCL_ERROR;
		}
		error = dbi_odbc_Catalog(interp,dbdata->hdbc,CATALOG_SQLForeignKeys,dbdata->defnullvalue,objc-2,objv+2);
		return TCL_OK;
	case SQLSpecialColumns:
		if (objc != 8) {
			Tcl_WrongNumArgs(interp, 2, objv, "type catalog schema table scope nullable");
			return TCL_ERROR;
		}
		error = dbi_odbc_Catalog(interp,dbdata->hdbc,CATALOG_SQLSpecialColumns,dbdata->defnullvalue,objc-2,objv+2);
		return TCL_OK;
	case SQLStatistics:
		if (objc != 7) {
			Tcl_WrongNumArgs(interp, 2, objv, "catalog schema table unique quick");
			return TCL_ERROR;
		}
		error = dbi_odbc_Catalog(interp,dbdata->hdbc,CATALOG_SQLStatistics,dbdata->defnullvalue,objc-2,objv+2);
		return TCL_OK;
	case SQLTables:
		if (objc != 6) {
			Tcl_WrongNumArgs(interp, 2, objv, "catalog schema table tabletype");
			return TCL_ERROR;
		}
		error = dbi_odbc_Catalog(interp,dbdata->hdbc,CATALOG_SQLTables,dbdata->defnullvalue,objc-2,objv+2);
		return TCL_OK;
	case SQLProcedures:
		if (objc != 5) {
			Tcl_WrongNumArgs(interp, 2, objv, "catalog schema procedure");
			return TCL_ERROR;
		}
		error = dbi_odbc_Catalog(interp,dbdata->hdbc,CATALOG_SQLProcedures,dbdata->defnullvalue,objc-2,objv+2);
		return TCL_OK;
	case SQLProcedureColumns:
		if (objc != 6) {
			Tcl_WrongNumArgs(interp, 2, objv, "catalog schema procedure column");
			return TCL_ERROR;
		}
		error = dbi_odbc_Catalog(interp,dbdata->hdbc,CATALOG_SQLProcedureColumns,dbdata->defnullvalue,objc-2,objv+2);
		return TCL_OK;
	case SQLGetinfo:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "infotype");
			return TCL_ERROR;
		}
		return dbi_odbc_SQLGetInfo(interp,dbdata,objv[2]);
	case Supports:
		if (objc == 2) {
			return dbi_odbc_Supports(interp,dbdata,NULL);
		} if (objc == 3) {
			return dbi_odbc_Supports(interp,dbdata,objv[2]);
		} else {
			Tcl_WrongNumArgs(interp, 2, objv, "?keyword?");
			return TCL_ERROR;
		}
	}
	return error;
}

int dbi_odbc_Destroy(
	ClientData clientdata)
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)clientdata;
	dbi_odbc_Close(dbdata);
	if (dbdata->defnullvalue != NULL) {Tcl_DecrRefCount(dbdata->defnullvalue);}
	Tcl_Free((char *)dbdata);
	Tcl_DeleteExitHandler((Tcl_ExitProc *)dbi_odbc_Destroy, clientdata);
	return TCL_OK;
}

static int dbi_num = 0;
int Dbi_odbc_DoNewDbObjCmd(
	dbi_odbc_Data *dbdata,
	Tcl_Interp *interp,
	Tcl_Obj *dbi_nameObj)
{
	char buffer[40];
	char *dbi_name;
	dbdata->interp = interp;
	dbdata->hasconn = 0;
	dbdata->defnullvalue = Tcl_NewObj();
	dbdata->dbms_name = NULL;
	dbdata->dbms_ver = NULL;
	dbdata->autocommit = 1;
	dbdata->trans = 0;
	dbdata->clones = NULL;
	dbdata->clonesnum = 0;
	dbdata->parent = NULL;
	dbi_odbc_initresult(&(dbdata->result));
	Tcl_IncrRefCount(dbdata->defnullvalue);
	if (dbi_nameObj == NULL) {
		dbi_num++;
		sprintf(buffer,"::dbi::odbc::dbi%d",dbi_num);
		dbi_nameObj = Tcl_NewStringObj(buffer,strlen(buffer));
	}
	dbi_name = Tcl_GetStringFromObj(dbi_nameObj,NULL);
	dbdata->token = Tcl_CreateObjCommand(interp,dbi_name,(Tcl_ObjCmdProc *)Dbi_odbc_DbObjCmd,
		(ClientData)dbdata,(Tcl_CmdDeleteProc *)dbi_odbc_Destroy);
	Tcl_CreateExitHandler((Tcl_ExitProc *)dbi_odbc_Destroy, (ClientData)dbdata);
	Tcl_SetObjResult(interp,dbi_nameObj);
	return TCL_OK;
}

int Dbi_odbc_NewDbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_odbc_Data *dbdata;
	int error;
	if ((objc < 1)||(objc > 2)) {
		Tcl_WrongNumArgs(interp,1,objv,"?dbName?");
		return TCL_ERROR;
	}
	dbdata = (dbi_odbc_Data *)Tcl_Alloc(sizeof(dbi_odbc_Data));
	if (objc == 2) {
		error = Dbi_odbc_DoNewDbObjCmd(dbdata,interp,objv[1]);
	} else {
		error = Dbi_odbc_DoNewDbObjCmd(dbdata,interp,NULL);
	}
	if (error) {
		Tcl_Free((char *)dbdata);
	}
	return error;
}

void dbi_odbc_exit(ClientData clientData)
{
	SQLFreeEnv(dbi_odbc_env);
}

int Dbi_odbc_Clone(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	Tcl_Obj *name)
{
	SQLHSTMT hstmt;
	dbi_odbc_Data *parent = NULL;
	dbi_odbc_Data *clone_dbdata = NULL;
	long erg;
	int error;
	parent = dbdata;
	while (parent->parent != NULL) {parent = parent->parent;}
	clone_dbdata = (dbi_odbc_Data *)Tcl_Alloc(sizeof(dbi_odbc_Data));
	error = Dbi_odbc_DoNewDbObjCmd(clone_dbdata,interp,name);
	if (error) {Tcl_Free((char *)clone_dbdata);return TCL_ERROR;}
	name = Tcl_GetObjResult(interp);
	parent->clonesnum++;
	parent->clones = (Tcl_Command *)Tcl_Realloc((char *)parent->clones,parent->clonesnum*sizeof(Tcl_Command *));
	parent->clones[parent->clonesnum-1] = clone_dbdata->token;
	clone_dbdata->parent = parent;
	clone_dbdata->hasconn = 1;
	clone_dbdata->hdbc = parent->hdbc;
	clone_dbdata->dbms_name = parent->dbms_name;
	clone_dbdata->dbms_ver = parent->dbms_ver;
	clone_dbdata->supportpos = parent->supportpos;
	erg=SQLAllocHandle(SQL_HANDLE_STMT, dbdata->hdbc, &hstmt);
	if ((erg != SQL_SUCCESS) && (erg != SQL_SUCCESS_WITH_INFO)) {
		Tcl_AppendResult(interp,"ODBC Error allocating statement handle",NULL);
		dbi_odbc_error(interp,erg,SQL_NULL_HDBC,dbdata->hstmt);
		goto error;
	}
	clone_dbdata->hstmt = hstmt;
	return TCL_OK;
	error:
		Tcl_DeleteCommandFromToken(interp,clone_dbdata->token);
		Tcl_Free((char *)clone_dbdata);
		return TCL_ERROR;
}

int Dbi_odbc_Init(interp)
	Tcl_Interp *interp;		/* Interpreter to add extra commands */
{
	long erg;

#ifdef USE_TCL_STUBS
	if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
		return TCL_ERROR;
	}
#endif
	erg = SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&(dbi_odbc_env));
	if ((erg != SQL_SUCCESS) && (erg != SQL_SUCCESS_WITH_INFO))	{
		Tcl_AppendResult(interp, "Could not allocate environment handle",NULL);
		return TCL_ERROR;
	}
	erg=SQLSetEnvAttr(dbi_odbc_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); 
	if ((erg != SQL_SUCCESS) && (erg != SQL_SUCCESS_WITH_INFO))	{
		Tcl_AppendResult(interp, "Error SetEnv",NULL);
		SQLFreeHandle(SQL_HANDLE_ENV, dbi_odbc_env);
		return TCL_ERROR;
	}
	Tcl_CreateExitHandler(dbi_odbc_exit, (ClientData) 0);
	Tcl_CreateObjCommand(interp,"dbi_odbc",(Tcl_ObjCmdProc *)Dbi_odbc_NewDbObjCmd,
		(ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
	Tcl_Eval(interp,"");
	return TCL_OK;
}
