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
		dbi_odbc_error(interp,rc,SQL_NULL_HDBC,dbdata->hstmt);
		if (buffer != fbuffer) {Tcl_Free(buffer);}
		return TCL_ERROR;
	}
	*result = Tcl_NewStringObj(buffer,reallen);
	if (buffer != fbuffer) {Tcl_Free(buffer);}
	return TCL_OK;
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
	rc = SQLError(dbi_odbc_env, hdbc, hstmt, 
		(UCHAR*) SqlState, &NativeError, (UCHAR*) SqlMessage, 
		SQL_MAX_MESSAGE_LENGTH-1, &Available);
	if (rc != SQL_ERROR) {
		Tcl_AppendResult(interp, SqlMessage, NULL);
	} else {
		Tcl_AppendResult(interp, "no error message", NULL);
	}
}

int dbi_odbc_Open(
	Tcl_Interp *interp,
	Dbi *db,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)db->dbdata;
	char *ds, *user = "", *pw = "", *string;
	long erg;
	int dslen=0, userlen=0, pwlen=0,stringlen,i;
	if (dbdata->hasconn != 0) {
		Tcl_AppendResult(interp,"dbi object has open connection, close first", NULL);
		return TCL_ERROR;
	}
	if (objc < 3) {
		Tcl_WrongNumArgs(interp,2,objv,"datasource ?-user user? ?-password password? ?-host host? ?-hostaddr hostaddress? ?-port port? ?-options options? ?-tty tty?");
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
	SQLSetConnectAttr(dbdata->hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0);
	ds = Tcl_GetStringFromObj(objv[2],&dslen);
	erg = SQLConnect(dbdata->hdbc, (SQLCHAR*) ds, dslen,(SQLCHAR*) user, userlen,
		(SQLCHAR*) pw, pwlen);
	if (erg == SQL_ERROR)	{
		Tcl_AppendResult(interp,"ODBC Error SQLConnect: ",NULL);
		dbi_odbc_error(interp,erg,dbdata->hdbc,SQL_NULL_HSTMT);
		return TCL_ERROR;
	}
	dbdata->hasconn = 1;
	return TCL_OK;
}

int dbi_odbc_Exec(
	Tcl_Interp *interp,
	Dbi *db,
	Tcl_Obj *cmd,
	int usefetch,
	Tcl_Obj *nullvalue)
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)db->dbdata;
	RETCODE rc;
	SQLHSTMT hstmt;
	SQLINTEGER rowcount;
	char *command;
	long erg;
	int error, commandlen;
	if (dbdata->hasconn == 0) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		return TCL_ERROR;
	}
	command = Tcl_GetStringFromObj(cmd,&commandlen);
	error = dbi_odbc_clearresult(interp,db);
	if (error) {return error;}
	erg=SQLAllocHandle(SQL_HANDLE_STMT, dbdata->hdbc, &hstmt);
	if ((erg != SQL_SUCCESS) && (erg != SQL_SUCCESS_WITH_INFO)) {
		Tcl_AppendResult(interp,"ODBC Error allocating statement handle",NULL);
		dbi_odbc_error(interp,erg,SQL_NULL_HDBC,dbdata->hstmt);
		return TCL_ERROR;
	}
	dbdata->hstmt = hstmt;
	erg=SQLExecDirect(hstmt,command,commandlen);   
	switch (erg) {
		case SQL_SUCCESS:
		case SQL_SUCCESS_WITH_INFO: 
			error = dbi_odbc_initresult(interp,db);
			if (error) {return error;}
			Tcl_ResetResult(interp);
			if (dbdata->nfields != 0) {
				if (!usefetch) {
					error = dbi_odbc_ToResult(interp,db,nullvalue);
					if (error) {goto error;}
				}
			}
			break;
		default:
			Tcl_AppendResult(interp,"database error executing command \"",
				Tcl_GetStringFromObj(cmd,NULL), "\":\n", NULL);
			dbi_odbc_error(interp,erg,SQL_NULL_HDBC,dbdata->hstmt);
			goto error;
	}
	if (!usefetch) {
		error = dbi_odbc_clearresult(interp,db);
		if (error) {return error;}
	}
	dbdata->t = 0;
	return TCL_OK;
	error:
		return TCL_ERROR;
}

int dbi_odbc_Fetch(
	Tcl_Interp *interp,
	Dbi *db,
	int fetch_option,
	int t,
	int f,
	Tcl_Obj *nullvalue)
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)db->dbdata;
	Tcl_Obj *result = NULL, *line = NULL, *element = NULL;
	RETCODE rc;
	char *string;
	int error,stringlen,i;
	int nfields = dbdata->nfields;
	if (dbdata->resultbuffer == NULL) {
		Tcl_AppendResult(interp, "no result available: invoke exec method with -usefetch option first", NULL);
		return TCL_ERROR;
	}
	if (f >= nfields) {
		Tcl_Obj *buffer;
		buffer = Tcl_NewIntObj(t);
		Tcl_AppendResult(interp, "field ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
		Tcl_DecrRefCount(buffer);
		return TCL_ERROR;
	}
	if (t != -1) {
		Tcl_AppendResult(interp,"odbc type: positioning for fetch not supported",NULL);
		return TCL_ERROR;
	}
	switch (fetch_option) {
		case DBI_FETCH_DATA:
			if (nullvalue == NULL) {
				nullvalue = dbdata->defnullvalue;
			}
			rc = SQLFetch(dbdata->hstmt);
			switch (rc) {
				case SQL_NO_DATA_FOUND:
					{
						Tcl_Obj *buffer;
						buffer = Tcl_NewIntObj(dbdata->t);
						Tcl_AppendResult(interp, "line ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
						Tcl_DecrRefCount(buffer);
						return TCL_ERROR;
					}
				case SQL_ERROR:
				case SQL_INVALID_HANDLE:
					Tcl_AppendResult(interp,"ODBC Error during fetch: ",NULL);
					dbi_odbc_error(interp,rc,SQL_NULL_HDBC,dbdata->hstmt);
					return TCL_ERROR;
			}
			dbdata->t++;
			if (f == -1) {
				error = dbi_odbc_GetRow(interp,db,nullvalue,&line);
				if (error) {return TCL_ERROR;}
				Tcl_SetObjResult(interp, line);
			} else {
				error = dbi_odbc_GetOne(interp,db,f,&element);
				if (error) {return TCL_ERROR;}
				if (element == NULL) {
					element = nullvalue;
				}
				Tcl_SetObjResult(interp, element);
			}
			break;
		case DBI_FETCH_LINES:
			Tcl_AppendResult(interp,"odbc type: fetch -lines not supported", NULL);
			return TCL_ERROR;
		case DBI_FETCH_FIELDS:
			line = Tcl_NewListObj(0,NULL);
			for (i=0; i<dbdata->nfields; ++i) {
				error = dbi_sql_colname(interp,dbdata,i+1,&element);
				if (error) {Tcl_DecrRefCount(line);return error;}
				error = Tcl_ListObjAppendElement(interp,line, element);
				if (error) {Tcl_DecrRefCount(line);Tcl_DecrRefCount(element);return TCL_ERROR;}
			}
			Tcl_SetObjResult(interp, line);
			break;
		case DBI_FETCH_CLEAR:
			error = dbi_odbc_clearresult(interp,db);
			if (error) {return TCL_ERROR;}
			break;
		case DBI_FETCH_ISNULL:
			Tcl_AppendResult(interp,"odbc type: fetch -isnull not supported", NULL);
			return TCL_ERROR;
	}
	return TCL_OK;
}

int dbi_odbc_Close(
	Tcl_Interp *interp,
	Dbi *db)
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)db->dbdata;
	if (dbdata->hasconn == 0) return TCL_OK;
	SQLDisconnect(dbdata->hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC,dbdata->hdbc);
	dbdata->hasconn = 0;
	return TCL_OK;
}

int dbi_odbc_Tables(
	Tcl_Interp *interp,
	Dbi *db)
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)db->dbdata;
	RETCODE rc;
       rc = SQLTables(dbdata->hstmt, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS);
	if (rc == SQL_ERROR) {
		Tcl_AppendResult(interp,"ODBC Error getting tables: ",NULL);
		dbi_odbc_error(interp,rc,SQL_NULL_HDBC,dbdata->hstmt);
		return TCL_ERROR;
	}
	return TCL_OK;
}

int dbi_odbc_Destroy(
	Dbi *db)
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)db->dbdata;
	if (dbdata->resultbuffer != NULL) Tcl_Free((char *)dbdata->resultbuffer);
	Tcl_DecrRefCount(dbdata->defnullvalue);
	Tcl_Free((char *)dbdata);
	return TCL_OK;
}

int dbi_odbc_Create(
	Tcl_Interp *interp,
	Dbi *db)
{
	dbi_odbc_Data *dbdata;
	long erg;
	dbdata = (dbi_odbc_Data *)Tcl_Alloc(sizeof(dbi_odbc_Data));
	dbdata->hasconn = 0;
	dbdata->resultbuffer = NULL;
	dbdata->defnullvalue = Tcl_NewObj();
	Tcl_IncrRefCount(dbdata->defnullvalue);
	/* allocate Environment handle and register version */
	db->dbdata = (ClientData)dbdata;
	db->open = dbi_odbc_Open;
/*	db->admin = dbi_odbc_Admin;*/
/*	db->configure = dbi_odbc_Configure;*/
	db->tables = dbi_odbc_Tables;
	db->exec = dbi_odbc_Exec;
	db->fetch = dbi_odbc_Fetch;
	db->close = dbi_odbc_Close;
	db->destroy = dbi_odbc_Destroy;
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
	dbi_CreateType(interp,"odbc",dbi_odbc_Create);
	return TCL_OK;
}
