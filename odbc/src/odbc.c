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
#include "dbi.h"
#include "odbc.h"

static SQLHENV dbi_odbc_env;

/******************************************************************/

void dbi_odbc_error(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
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
	rc = SQLError(dbi_odbc_env, hdbc, hstmt, 
		(UCHAR*) SqlState, &NativeError, (UCHAR*) SqlMessage, 
		SQL_MAX_MESSAGE_LENGTH-1, &Available);
	if (rc != SQL_ERROR) {
		Tcl_AppendResult(interp, SqlMessage, NULL);
	} else {
		Tcl_AppendResult(interp, "no error message", NULL);
	}
	if ((dbdata->autocommit)&&(dbdata->trans)) {
		rc = SQLEndTran(SQL_HANDLE_DBC,dbdata->hdbc,SQL_ROLLBACK);
		dbdata->trans = 0;
		if (rc) {
			Tcl_AppendResult(interp,"error rolling back transaction:\n", NULL);
			dbi_odbc_error(interp,dbdata,rc,SQL_NULL_HDBC,SQL_NULL_HSTMT);
		}
/* fprintf(stdout," ---- rolled back by error ---- \n");fflush(stdout); */
	}
}

int dbi_sql_colname(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	int col,
	Tcl_Obj **result
)
{
	RETCODE rc;
	char fbuffer[256];
	char *buffer=fbuffer;
	SWORD reallen;
	rc = SQLColAttributes(dbdata->hstmt, (UWORD)(col), SQL_COLUMN_NAME, 
		buffer, 256, &reallen, NULL);
	if (rc == SQL_SUCCESS_WITH_INFO) {
		buffer = (void *)Tcl_Alloc(reallen);
		rc = SQLColAttributes(dbdata->hstmt, (UWORD)(col), SQL_COLUMN_NAME, 
			buffer, reallen, &reallen, NULL);
	}
	if (rc == SQL_ERROR) {
		Tcl_AppendResult(interp,"ODBC Error getting column name: ",NULL);
		dbi_odbc_error(interp,dbdata,rc,SQL_NULL_HDBC,dbdata->hstmt);
		if (buffer != fbuffer) {Tcl_Free(buffer);}
		return TCL_ERROR;
	}
	*result = Tcl_NewStringObj(buffer,reallen);
	if (buffer != fbuffer) {Tcl_Free(buffer);}
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
	int dslen=0, userlen=0, pwlen=0,stringlen,i,error;
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
		dbi_odbc_error(interp,dbdata,erg,dbdata->hdbc,SQL_NULL_HSTMT);
		return TCL_ERROR;
	}
	dbdata->hasconn = 1;
	erg=SQLAllocHandle(SQL_HANDLE_STMT, dbdata->hdbc, &hstmt);
	if ((erg != SQL_SUCCESS) && (erg != SQL_SUCCESS_WITH_INFO)) {
		Tcl_AppendResult(interp,"ODBC Error allocating statement handle",NULL);
		dbi_odbc_error(interp,dbdata,erg,SQL_NULL_HDBC,dbdata->hstmt);
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
	if (uint && SQL_CA1_ABSOLUTE) {
		dbdata->supportpos = 1;
	} else {
		dbdata->supportpos = 0;
	}
	return TCL_OK;
}

int dbi_odbc_Transaction_Start(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata)
{
	RETCODE rc;
	if ((dbdata->autocommit)&&(dbdata->trans)) {
		rc = SQLEndTran(SQL_HANDLE_DBC,dbdata->hdbc,SQL_COMMIT);
		if (rc) {
			Tcl_AppendResult(interp,"error starting transaction:\n", NULL);
			dbi_odbc_error(interp,dbdata,rc,dbdata->hdbc,SQL_NULL_HSTMT);
			return TCL_ERROR;
		}
	}
	dbdata->trans = 1;
	return TCL_OK;
}

int dbi_odbc_Transaction_Commit(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata)
{
	RETCODE rc;
	if ((dbdata->autocommit)&&(dbdata->trans)) {
		rc = SQLEndTran(SQL_HANDLE_DBC,dbdata->hdbc,SQL_COMMIT);
		if (rc) {
			Tcl_AppendResult(interp,"error committing transaction:\n", NULL);
			dbi_odbc_error(interp,dbdata,rc,dbdata->hdbc,SQL_NULL_HSTMT);
			return TCL_ERROR;
		}
/* fprintf(stdout," ---- committed ---- \n");fflush(stdout); */
		dbdata->trans = 0;
	}
	return TCL_OK;
}

int dbi_odbc_Transaction_Rollback(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata)
{
	RETCODE rc;
	if ((dbdata->autocommit)&&(dbdata->trans)) {
		rc = SQLEndTran(SQL_HANDLE_DBC,dbdata->hdbc,SQL_ROLLBACK);
		if (rc) {
			Tcl_AppendResult(interp,"error rolling back transaction:\n", NULL);
			dbi_odbc_error(interp,dbdata,rc,dbdata->hdbc,SQL_NULL_HSTMT);
			return TCL_ERROR;
		}
/* fprintf(stdout," ---- rolled back ---- \n");fflush(stdout); */
		dbdata->trans = 0;
	}
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
	RETCODE rc;
	SQLHSTMT hstmt = dbdata->hstmt;
	SQLINTEGER rowcount;
	SQLSMALLINT NumParams;
	Tcl_Obj *tempstring;
	long erg;
	int error;
	if (!DB_OPENCONN(dbdata)) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		return TCL_ERROR;
	}
	if (cmdlen == 0) {return TCL_OK;}
	error = dbi_odbc_clearresult(interp,dbdata);
	if (error) {return error;}
	erg = SQLPrepare(hstmt,cmdstring,cmdlen);
	if (erg != SQL_SUCCESS) {
		Tcl_AppendResult(interp,"ODBC Error preparing statement:\n",NULL);
		dbi_odbc_error(interp,dbdata,erg,SQL_NULL_HDBC,dbdata->hstmt);
		goto error;
	}
	erg = SQLNumParams(hstmt, &NumParams);
	if (erg != SQL_SUCCESS) {
		Tcl_AppendResult(interp,"ODBC Error getting number of parameters:\n",NULL);
		dbi_odbc_error(interp,dbdata,erg,SQL_NULL_HDBC,dbdata->hstmt);
		goto error;
	}
	if ((int)NumParams != objc) {
		Tcl_AppendResult(interp,"wrong number of arguments given to exec", NULL);
		goto error;
	}
	if (NumParams) {
		SQLSMALLINT i,DataType, DecimalDigits, Nullable;
		SQLUINTEGER ParamSize;
		SQLINTEGER *sqlarglen = NULL;
		char *argstring;
		int arglen,retlen;
		sqlarglen = (SQLINTEGER *)Tcl_Alloc(NumParams*sizeof(SQLINTEGER));
		for (i = 0; i < NumParams; i++) {
			/* Describe the parameter. */
			SQLDescribeParam(hstmt, i + 1, &DataType, &ParamSize, &DecimalDigits, &Nullable);
			argstring = Tcl_GetStringFromObj(objv[(int)i],&arglen);
			sqlarglen[i] = (SQLINTEGER) arglen;
			/* Bind the memory to the parameter. We only have input parameters. */
			SQLBindParameter(hstmt, i + 1, SQL_PARAM_INPUT, SQL_C_CHAR, DataType, ParamSize,
			 	  DecimalDigits, argstring, arglen, sqlarglen+i);
		}
		dbdata->rc = SQLExecute(hstmt);
		SQLFreeStmt(hstmt,SQL_RESET_PARAMS);
		Tcl_Free((char *)sqlarglen);
	} else {
		dbdata->rc = SQLExecute(hstmt);
	}
	switch (dbdata->rc) {
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO: 
			break;
		default:
			dbi_odbc_error(interp,dbdata,erg,SQL_NULL_HDBC,dbdata->hstmt);
			goto error;
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
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	RETCODE rc;
	SQLHSTMT hstmt = dbdata->hstmt;
	SQLINTEGER rowcount;
	SQLSMALLINT NumParams;
	char *command;
	long erg;
	int *lens = NULL;
	int error, commandlen,number,pos,i,showcmd = 0;
	if (!DB_OPENCONN(dbdata)) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		return TCL_ERROR;
	}
/*	erg=SQLAllocHandle(SQL_HANDLE_STMT, dbdata->hdbc, &hstmt);
	if ((erg != SQL_SUCCESS) && (erg != SQL_SUCCESS_WITH_INFO)) {
		Tcl_AppendResult(interp,"ODBC Error allocating statement handle",NULL);
		dbi_odbc_error(interp,dbdata,erg,SQL_NULL_HDBC,dbdata->hstmt);
		return TCL_ERROR;
	}
*/
	dbdata->hstmt = hstmt;
	command = Tcl_GetStringFromObj(cmd,&commandlen);
	if (commandlen == 0) {return TCL_OK;}
/*
fprintf(stdout,"************************************\n");fflush(stdout);
fprintf(stdout,"%s\n",command);fflush(stdout);
fprintf(stdout," ---- trans: %d autocommit: %d\n",dbdata->trans,dbdata->autocommit);fflush(stdout);
*/
	error = dbi_odbc_clearresult(interp,dbdata);
	if (error) {return error;}
	if (dbi_odbc_Transaction_Start(interp,dbdata) != TCL_OK) {goto error;}
	error = dbi_odbc_SplitSql(command,commandlen,&number,&lens);
	if (error) {goto error;}
	if (number == 1) {
		error = dbi_odbc_Process_statement(interp,dbdata,commandlen,command,nullvalue,objc,objv);
		if (error) {dbdata->autocommit = 1;goto error;}
	} else {
		if (objc != 0) {
			Tcl_AppendResult(interp,"multiple commands cannot take arguments\n", NULL);
			goto error;
		}
		for(i = 0 ; i < number ; i++) {
			/* fprintf(stdout,"%d: %*.*s\n",lens[i],lens[i],lens[i],command);fflush(stdout); */
			error = dbi_odbc_Process_statement(interp,dbdata,lens[i],command,nullvalue,0,NULL);
			if (error) {dbdata->autocommit = 1;goto error;}
			command += lens[i]+1;
		}
		Tcl_Free((char *)lens); lens = NULL;
	}
	switch (dbdata->rc) {
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO: 
			error = dbi_odbc_initresult(interp,dbdata);
			if (error) {return error;}
			Tcl_ResetResult(interp);
			if (dbdata->nfields != 0) {
				if (!usefetch) {
					error = dbi_odbc_ToResult(interp,dbdata,nullvalue);
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
			dbi_odbc_error(interp,dbdata,erg,SQL_NULL_HDBC,dbdata->hstmt);
			showcmd = 1;
			goto error;
	}
	if (!usefetch) {
		error = dbi_odbc_clearresult(interp,dbdata);
		if (error) {return error;}
	}
	dbdata->tuple = 0;
	return TCL_OK;
	error:
		if (lens == NULL) {Tcl_Free((char *)lens);}
		if (showcmd) {
			Tcl_AppendResult(interp," while executing command: \"",	Tcl_GetStringFromObj(cmd,NULL), "\"", NULL);
		}
		return TCL_ERROR;
}

int dbi_odbc_Fetch(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
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
	int i, ituple = -1, ifield = -1;
	Tcl_Obj *result = NULL, *line = NULL, *element = NULL;
	RETCODE rc;
	char *string;
	int error,stringlen;
	int nfields = dbdata->nfields;
	int ntuples = dbdata->ntuples;
	if (dbdata->resultbuffer == NULL) {
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
			Tcl_SetObjResult(interp,Tcl_NewIntObj(dbdata->ntuples));
			return TCL_OK;
		case Fields:
			line = Tcl_NewListObj(0,NULL);
			for (i=0; i<dbdata->nfields; ++i) {
				error = dbi_sql_colname(interp,dbdata,i+1,&element);
				if (error) {Tcl_DecrRefCount(line);return error;}
				error = Tcl_ListObjAppendElement(interp,line, element);
				if (error) {Tcl_DecrRefCount(line);Tcl_DecrRefCount(element);return TCL_ERROR;}
			}
			Tcl_SetObjResult(interp, line);
			return TCL_OK;
		case Clear:
			error = dbi_odbc_clearresult(interp,dbdata);
			return error;
		case Pos:
			Tcl_SetObjResult(interp,Tcl_NewIntObj(dbdata->tuple));
			return TCL_OK;
	}
	/*
	 * fetch data
	 * ----------
	 */
	if (dbdata->supportpos) {
		if (ituple >= dbdata->ntuples) {
			Tcl_Obj *buffer;
			buffer = Tcl_NewIntObj(ituple);
			Tcl_AppendResult(interp, "line ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
			Tcl_DecrRefCount(buffer);
			return TCL_ERROR;
		}
	} else if (ituple != -1) {
		Tcl_AppendResult(interp,"absolute positioning for fetch not supported by driver",NULL);
		return TCL_ERROR;
	}
	if (ifield >= nfields) {
		Tcl_Obj *buffer;
		buffer = Tcl_NewIntObj(ifield);
		Tcl_AppendResult(interp, "field ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
		Tcl_DecrRefCount(buffer);
		return TCL_ERROR;
	}
	switch (fetch_option) {
		case Data:
			if (nullvalue == NULL) {
				nullvalue = dbdata->defnullvalue;
			}
			if (ituple == -1) {
				rc = SQLFetch(dbdata->hstmt);
			} else if (ituple != dbdata->tuple) {
				rc = SQLFetchScroll(dbdata->hstmt,SQL_FETCH_ABSOLUTE,ituple+1);
				dbdata->tuple = ituple;
			}
			switch (rc) {
				case SQL_NO_DATA_FOUND:
					{
						Tcl_Obj *buffer;
						buffer = Tcl_NewIntObj(dbdata->tuple);
						Tcl_AppendResult(interp, "line ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
						Tcl_DecrRefCount(buffer);
						return TCL_ERROR;
					}
				case SQL_ERROR:
				case SQL_INVALID_HANDLE:
					Tcl_AppendResult(interp,"ODBC Error during fetch: ",NULL);
					dbi_odbc_error(interp,dbdata,rc,SQL_NULL_HDBC,dbdata->hstmt);
					return TCL_ERROR;
			}
			dbdata->tuple++;
			if (ifield == -1) {
				error = dbi_odbc_GetRow(interp,dbdata,nullvalue,&line);
				if (error) {return TCL_ERROR;}
				Tcl_SetObjResult(interp, line);
			} else {
				error = dbi_odbc_GetOne(interp,dbdata,ifield,&element);
				if (error) {return TCL_ERROR;}
				if (element == NULL) {
					element = nullvalue;
				}
				Tcl_SetObjResult(interp, element);
			}
			break;
		case Isnull:
			Tcl_AppendResult(interp,"odbc type: fetch isnull not supported", NULL);
			return TCL_ERROR;
	}
	return TCL_OK;
}

int dbi_odbc_Close(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata)
{
	if (!DB_OPENCONN(dbdata)) return TCL_OK;
	if (dbdata->trans) {
		SQLEndTran(SQL_HANDLE_DBC,dbdata->hdbc,SQL_ROLLBACK);
		dbdata->trans = 0;
	}
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
	return TCL_OK;
}

int dbi_odbc_Fields(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	Tcl_Obj *table)
{
	RETCODE rc;
	rc = SQLTables(dbdata->hstmt, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS);
	if (rc == SQL_ERROR) {
		Tcl_AppendResult(interp,"ODBC Error getting tables: ",NULL);
		dbi_odbc_error(interp,dbdata,rc,SQL_NULL_HDBC,dbdata->hstmt);
		return TCL_ERROR;
	}
	return TCL_OK;
}

int dbi_odbc_Tables(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata)
{
	RETCODE rc;
	rc = SQLTables(dbdata->hstmt, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS);
	if (rc == SQL_ERROR) {
		Tcl_AppendResult(interp,"ODBC Error getting tables: ",NULL);
		dbi_odbc_error(interp,dbdata,rc,SQL_NULL_HDBC,dbdata->hstmt);
		return TCL_ERROR;
	}
	return TCL_OK;
}

int dbi_odbc_Interface(
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	int i;
	static char *interfaces[] = {
		"dbi", "0.1", "dbi/blob", "0.1",
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

int dbi_odbc_Serial(
	Tcl_Interp *interp,
	dbi_odbc_Data *dbdata,
	Tcl_Obj *subcmd,
	Tcl_Obj *table,
	Tcl_Obj *field,
	Tcl_Obj *current)
{
	Tcl_Obj *cmd,*temp;
	char *name;
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
			Tcl_AppendStringsToObj(cmd,"_add ",NULL);
			break;
		case Delete:
			Tcl_AppendStringsToObj(cmd,"_delete ",NULL);
			current = NULL;
			break;
		case Set:
			Tcl_AppendStringsToObj(cmd,"_set ",NULL);
			break;
		case Next:
			Tcl_AppendStringsToObj(cmd,"_next ",NULL);
			break;
	}
	name = Tcl_GetCommandName(interp,dbdata->token);
	Tcl_AppendStringsToObj(cmd,Tcl_GetStringFromObj(dbdata->namespace,NULL),"::",name,NULL);
	if (error) {Tcl_DecrRefCount(cmd);return error;}
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
		"destroy", "serial",
		(char *) NULL};
	enum ISubCmdIdx {
		Interface, Open, Exec, Fetch, Close,
		Info, Tables, Fields, Tableinfo,
		Begin, Commit, Rollback,
		Destroy, Serial,
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
		return dbi_odbc_Open(interp,dbdata,objc,objv);
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
				Tcl_AppendResult(interp,"dbi_odbc: -command not supported",NULL);
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
		return dbi_odbc_Exec(interp,dbdata,objv[i],usefetch,nullvalue,objc-i-1,objv+i+1);
		}
	case Fetch:
		return dbi_odbc_Fetch(interp,dbdata, objc, objv);
/*
	case Info:
		return dbi_odbc_Info(interp,dbdata,objc-2,objv+2);
*/
	case Tables:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_odbc_Tables(interp,dbdata);
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
		return dbi_odbc_Close(interp,dbdata);
	case Begin:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (!DB_OPENCONN(dbdata)) {
			Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
			return error;
		}
		error = dbi_odbc_Transaction_Start(interp,dbdata);
		if (error) {dbi_odbc_error(interp,dbdata,error,dbdata->hdbc,SQL_NULL_HSTMT);return error;}
		dbdata->autocommit = 0;
		return TCL_OK;
	case Commit:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (!DB_OPENCONN(dbdata)) {
			Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
			return error;
		}
		dbdata->autocommit = 1;
		error = dbi_odbc_Transaction_Commit(interp,dbdata);
		if (error) {dbi_odbc_error(interp,dbdata,error,dbdata->hdbc,SQL_NULL_HSTMT);return error;}
		return TCL_OK;
	case Rollback:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (!DB_OPENCONN(dbdata)) {
			Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
			return error;
		}
		dbdata->autocommit = 1;
		error = dbi_odbc_Transaction_Rollback(interp,dbdata);
		if (error) {dbi_odbc_error(interp,dbdata,error,dbdata->hdbc,SQL_NULL_HSTMT);return error;}
		return TCL_OK;
	case Destroy:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		Tcl_DeleteCommandFromToken(interp,dbdata->token);
		return TCL_OK;
	case Serial:
		if (objc < 5) {
			Tcl_WrongNumArgs(interp, 1, objv, "serial option table field ?value?");
			return TCL_ERROR;
		}
		if (objc == 6) {
			return dbi_odbc_Serial(interp,dbdata,objv[2],objv[3],objv[4],objv[5]);
		} else {
			return dbi_odbc_Serial(interp,dbdata,objv[2],objv[3],objv[4],NULL);
		}
	}
	return error;
}

int dbi_odbc_Destroy(
	dbi_odbc_Data *dbdata)
{
	dbi_odbc_Close(NULL,dbdata);
	if (dbdata->resultbuffer != NULL) Tcl_Free((char *)dbdata->resultbuffer);
	Tcl_DecrRefCount(dbdata->defnullvalue);
	Tcl_DecrRefCount(dbdata->namespace);
	Tcl_Free((char *)dbdata);
	return TCL_OK;
}

static int dbi_num = 0;
int Dbi_odbc_NewDbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_odbc_Data *dbdata;
	Tcl_Obj *dbi_nameObj = NULL;
	char buffer[40];
	char *dbi_name;
	int start;
	if ((objc < 1)||(objc > 2)) {
		Tcl_WrongNumArgs(interp,2,objv,"?dbName?");
		return TCL_ERROR;
	}
	dbdata = (dbi_odbc_Data *)Tcl_Alloc(sizeof(dbi_odbc_Data));
	dbdata->hasconn = 0;
	dbdata->resultbuffer = NULL;
	dbdata->defnullvalue = Tcl_NewObj();
	dbdata->dbms_name = NULL;
	dbdata->dbms_ver = NULL;
	dbdata->autocommit = 1;
	dbdata->trans = 0;
	dbdata->nfields = -1;
	dbdata->ntuples = -1;
	dbdata->tuple = 0;
	Tcl_IncrRefCount(dbdata->defnullvalue);
	/*
		we are keeping the dbi namespace for future use. This means that said future use
		will not work any longer when the command is renamed to  different namespace, but 
		we cannot get the fully qualified (including namespaces) command name from the token
    */
	if (objc == 2) {
		dbi_nameObj = objv[1];
		start = 2;
		Tcl_EvalEx(interp,"namespace current",17,0);
		dbdata->namespace = Tcl_GetObjResult(interp);
	} else {
		dbi_num++;
		sprintf(buffer,"::dbi::dbi_odbc%d",dbi_num);
		dbi_nameObj = Tcl_NewStringObj(buffer,strlen(buffer));
		start = 2;
		dbdata->namespace = Tcl_NewStringObj("::dbi",5);
	}
	Tcl_IncrRefCount(dbdata->namespace);
	dbi_name = Tcl_GetStringFromObj(dbi_nameObj,NULL);
	dbdata->token = Tcl_CreateObjCommand(interp,dbi_name,(Tcl_ObjCmdProc *)Dbi_odbc_DbObjCmd,
		(ClientData)dbdata,(Tcl_CmdDeleteProc *)dbi_odbc_Destroy);
	Tcl_CreateExitHandler((Tcl_ExitProc *)dbi_odbc_Destroy, (ClientData)dbdata);
	Tcl_SetObjResult(interp,dbi_nameObj);
	return TCL_OK;
}

void dbi_odbc_exit(ClientData clientData)
{
	SQLFreeEnv(dbi_odbc_env);
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
