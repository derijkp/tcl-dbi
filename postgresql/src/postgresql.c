/*
 *       File:    mem.c
 *       Purpose: dbi extension to Tcl: postgresql backend
 *       Author:  Copyright (c) 1998 Peter De Rijk
 *
 *       See the file "README" for information on usage and redistribution
 *       of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <string.h>
#include <stdlib.h>
#include "postgresql.h"

/******************************************************************/

void dbi_Postgresql_NoticeProcessor(
	void *arg,
	const char *message)
{
}

int dbi_Postgresql_Open(
	Tcl_Interp *interp,
	Dbi *db,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_Postgresql_Data *dbdata = (dbi_Postgresql_Data *)db->dbdata;
	PGconn *conn = NULL;
	Tcl_Obj *conninfo=NULL;
	char *string;
	int len,i;
	if (dbdata->conn != NULL) {
		Tcl_AppendResult(interp,"dbi object has open connection, close first", NULL);
		goto error;
	}
	if (objc < 3) {
		Tcl_WrongNumArgs(interp,2,objv,"datasource ?-user user? ?-password password? ?-host host? ?-hostaddr hostaddress? ?-port port? ?-options options? ?-tty tty?");
		return TCL_ERROR;
	}
	conninfo = Tcl_NewStringObj("dbname='",8);
	Tcl_IncrRefCount(conninfo);
	string = Tcl_GetStringFromObj(objv[2],&len);
	Tcl_AppendToObj(conninfo,string,len);
	if ((objc-3)%2 != 0) {
		Tcl_WrongNumArgs(interp,2,objv,"datasource ?-user user? ?-password password? ?-host host? ?-hostaddr hostaddress? ?-port port? ?-options options? ?-tty tty?");
		goto error;
	}
	for (i = 3 ; i < objc ; i += 2) {
		Tcl_AppendToObj(conninfo,"' ",2);
		string = Tcl_GetStringFromObj(objv[i],&len);
		Tcl_AppendToObj(conninfo,string+1,len-1);
		Tcl_AppendToObj(conninfo,"='",2);
		string = Tcl_GetStringFromObj(objv[i+1],&len);
		Tcl_AppendToObj(conninfo,string,len);
	}
	Tcl_AppendToObj(conninfo,"'",1);
	conn = PQconnectdb(Tcl_GetStringFromObj(conninfo,NULL));
	if ((conn == NULL) || (PQstatus(conn) == CONNECTION_BAD))	{
		Tcl_AppendResult(interp,"connection to database \"",
			Tcl_GetStringFromObj(objv[2],NULL) , "\" failed:\n",
			PQerrorMessage(conn), NULL);
		goto error;
	}
	dbdata->conn = conn;
	Tcl_DecrRefCount(conninfo);
	PQsetNoticeProcessor(conn,dbi_Postgresql_NoticeProcessor,NULL);
	return TCL_OK;
	error:
		if (conn != NULL) PQfinish(conn);
		if (conninfo != NULL) Tcl_DecrRefCount(conninfo);
		return TCL_ERROR;
}

int dbi_Postgresql_ToResult(
	Tcl_Interp *interp,
	Dbi *db,
	PGresult *res,
	Tcl_Obj *nullvalue)
{
	Tcl_Obj *result = NULL, *line = NULL, *element = NULL;
	char *string;
	int t,f,error;
	int ntuples = PQntuples(res);
	int nfields = PQnfields(res);
	if (nullvalue == NULL) {
		nullvalue = Tcl_NewObj();
	}
	Tcl_IncrRefCount(nullvalue);
	result = Tcl_NewListObj(0,NULL);
	for (t = 0 ; t < ntuples ; t++) {
		line = Tcl_NewListObj(0,NULL);
		for (f = 0 ; f < nfields ; f++) {
			if (PQgetisnull(res,t,f)) {
				element = nullvalue;
			} else {
				string = PQgetvalue(res,t,f);
				element = Tcl_NewStringObj(string,strlen(string));
			}
			error = Tcl_ListObjAppendElement(interp,line, element);
			if (error) goto error;
			element = NULL;
		}
		error = Tcl_ListObjAppendElement(interp,result, line);
		if (error) goto error;
		line = NULL;
	}
	Tcl_SetObjResult(interp, result);
	Tcl_DecrRefCount(nullvalue);
	return TCL_OK;
	error:
		Tcl_DecrRefCount(nullvalue);
		if (result != NULL) Tcl_DecrRefCount(result);
		if (line != NULL) Tcl_DecrRefCount(line);
		if (element != NULL) Tcl_DecrRefCount(element);
		return TCL_ERROR;
}

int dbi_Postgresql_Exec(
	Tcl_Interp *interp,
	Dbi *db,
	Tcl_Obj *cmd,
	int usefetch,
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	PGresult *res = NULL;
	ExecStatusType status;
	dbi_Postgresql_Data *dbdata = (dbi_Postgresql_Data *)db->dbdata;
	int error;
	if (dbdata->conn == NULL) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		goto error;
	}
	res = PQexec(dbdata->conn, Tcl_GetStringFromObj(cmd,NULL));
	if (res == NULL) {
			Tcl_AppendResult(interp,"database error executing command \"",
				Tcl_GetStringFromObj(cmd,NULL), "\":\n",
				PQerrorMessage(dbdata->conn), NULL);
			goto error;
	}
	status = PQresultStatus(res);
	switch (status) {
		case PGRES_COMMAND_OK:
			Tcl_ResetResult(interp);
			Tcl_AppendResult(interp,PQcmdTuples(res),NULL);
			break;
		case PGRES_TUPLES_OK: 
			Tcl_ResetResult(interp);
			if (!usefetch) {
				error = dbi_Postgresql_ToResult(interp,db,res,nullvalue);
				if (error) {goto error;}
			}
			break;
		default:
			Tcl_AppendResult(interp,"database error executing command \"",
				Tcl_GetStringFromObj(cmd,NULL), "\":\n",
				PQresultErrorMessage(res), NULL);
			goto error;
	}
	dbdata->respos = 0;
	if (usefetch) {
		if (dbdata->res != NULL) {
			PQclear(dbdata->res);
		}
		dbdata->res = res;
	} else {
		PQclear(res);
	}
	return TCL_OK;
	error:
		if (res != NULL) PQclear(res);
		return TCL_ERROR;
}

int dbi_Postgresql_Fetch(
	Tcl_Interp *interp,
	Dbi *db,
	int fetch_option,
	int t,
	int f,
	Tcl_Obj *nullvalue)
{
	dbi_Postgresql_Data *dbdata = (dbi_Postgresql_Data *)db->dbdata;
	PGresult *res = dbdata->res;
	Tcl_Obj *result = NULL, *line = NULL, *element = NULL;
	char *string;
	int error,stringlen;
	int ntuples = PQntuples(res);
	int nfields = PQnfields(res);
	if (res == NULL) {
		Tcl_AppendResult(interp, "no result available: invoke exec method with -usefetch option first", NULL);
		return TCL_ERROR;
	}
	if (t == -1) {
		t = dbdata->respos;
	} else {
		dbdata->respos = t;
	}
	switch (fetch_option) {
		case DBI_FETCH_DATA:
		case DBI_FETCH_ISNULL:
		case DBI_FETCH_CURRENTLINE:
			if (t >= ntuples) {
				Tcl_Obj *buffer;
				buffer = Tcl_NewIntObj(t);
				Tcl_AppendResult(interp, "line ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
				Tcl_DecrRefCount(buffer);
				return TCL_ERROR;
			}
			if (f >= nfields) {
				Tcl_Obj *buffer;
				buffer = Tcl_NewIntObj(f);
				Tcl_AppendResult(interp, "field ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
				Tcl_DecrRefCount(buffer);
				return TCL_ERROR;
			}
	}
	switch (fetch_option) {
		case DBI_FETCH_DATA:
			if (nullvalue == NULL) {
				nullvalue = Tcl_NewObj();
			}
			Tcl_IncrRefCount(nullvalue);
			if (f == -1) {
				line = Tcl_NewListObj(0,NULL);
				for (f = 0 ; f < nfields ; f++) {
					if (PQgetisnull(res,t,f)) {
						element = nullvalue;
					} else {
						string = PQgetvalue(res,t,f);
						element = Tcl_NewStringObj(string,strlen(string));
					}
					error = Tcl_ListObjAppendElement(interp,line, element);
					if (error) {Tcl_DecrRefCount(nullvalue);goto error;}
					element = NULL;
				}
				Tcl_SetObjResult(interp, line);
				line = NULL;
				dbdata->respos++;
			} else {
				if (PQgetisnull(res,t,f)) {
					element = nullvalue;
				} else {
					string = PQgetvalue(res,t,f);
					element = Tcl_NewStringObj(string,strlen(string));
				}
				Tcl_SetObjResult(interp, element);
				line = NULL;
			}
			Tcl_DecrRefCount(nullvalue);
			break;
		case DBI_FETCH_LINES:
			Tcl_SetObjResult(interp,Tcl_NewIntObj(ntuples));
			break;
		case DBI_FETCH_FIELDS:
			line = Tcl_NewListObj(0,NULL);
			for (f = 0 ; f < nfields ; f++) {
				string = PQfname(res,f);
				element = Tcl_NewStringObj(string,strlen(string));
				error = Tcl_ListObjAppendElement(interp,line, element);
				if (error) goto error;
				element = NULL;
			}
			Tcl_SetObjResult(interp, line);
			line = NULL;
			break;
		case DBI_FETCH_CLEAR:
			PQclear(res);
			dbdata->res = NULL;
			break;
		case DBI_FETCH_ISNULL:
			Tcl_SetObjResult(interp,Tcl_NewIntObj(PQgetisnull(res,t,f)));
			break;
		case DBI_FETCH_CURRENTLINE:
			Tcl_SetObjResult(interp,Tcl_NewIntObj(dbdata->respos));
			break;
	}
	return TCL_OK;
	error:
		if (result != NULL) Tcl_DecrRefCount(result);
		if (line != NULL) Tcl_DecrRefCount(line);
		if (element != NULL) Tcl_DecrRefCount(element);
		return TCL_ERROR;
}

int dbi_Postgresql_Tables(
	Tcl_Interp *interp,
	Dbi *db)
{
	dbi_Postgresql_Data *dbdata = (dbi_Postgresql_Data *)db->dbdata;
	int error;
	Tcl_Obj *cmd = Tcl_NewStringObj("select relname from pg_class where relkind = 'r' and relname !~ '^pg_'",70);
	error = dbi_Postgresql_Exec(interp,db,cmd,0,NULL,0,(Tcl_Obj **)NULL);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Postgresql_Tableinfo(
	Tcl_Interp *interp,
	Dbi *db,
	Tcl_Obj *table,
	Tcl_Obj *varName,
	int type)
{
	dbi_Postgresql_Data *dbdata = (dbi_Postgresql_Data *)db->dbdata;
	Tcl_Obj *cmd,*temp;
	char *name;
	int error;
	name = Tcl_GetCommandName(interp,db->token);
	if (type == DBI_INFO_ALL) {
		cmd = Tcl_NewStringObj("::dbi::postgresql_tableinfo",-1);
	} else {
		cmd = Tcl_NewStringObj("::dbi::postgresql_fieldsinfo",-1);
	}
	Tcl_IncrRefCount(cmd);
	temp = Tcl_NewStringObj(name,-1);
	error = Tcl_ListObjAppendElement(interp,cmd,temp);
	if (error) {Tcl_DecrRefCount(cmd);Tcl_DecrRefCount(temp);return error;}
	error = Tcl_ListObjAppendElement(interp,cmd,table);
	if (error) {Tcl_DecrRefCount(cmd);return error;}
	if (type == DBI_INFO_ALL) {
		error = Tcl_ListObjAppendElement(interp,cmd,varName);
		if (error) {Tcl_DecrRefCount(cmd);return error;}
	}
	error = Tcl_EvalObj(interp,cmd);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Postgresql_Transaction(
	Tcl_Interp *interp,
	Dbi *db,
	int flag)
{
	PGresult *res = NULL;
	ExecStatusType status;
	dbi_Postgresql_Data *dbdata = (dbi_Postgresql_Data *)db->dbdata;
	char *cmd;
	int error;
	if (dbdata->conn == NULL) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		goto error;
	}
	switch (flag) {
		case TRANSACTION_BEGIN:
			cmd = "BEGIN";
			break;
		case TRANSACTION_COMMIT:
			cmd = "COMMIT";
			break;
		case TRANSACTION_ROLLBACK:
			cmd = "ROLLBACK";
			break;
	}
	res = PQexec(dbdata->conn, cmd);
	if (res == NULL) {
			Tcl_AppendResult(interp,"database error executing command \"",
				cmd, "\":\n",
				PQerrorMessage(dbdata->conn), NULL);
			goto error;
	}
	status = PQresultStatus(res);
	switch (status) {
		case PGRES_COMMAND_OK:
			Tcl_ResetResult(interp);
			Tcl_AppendResult(interp,PQcmdTuples(res),NULL);
			break;
		default:
			Tcl_AppendResult(interp,"database error executing command \"",
				cmd, "\":\n",
				PQresultErrorMessage(res), NULL);
			goto error;
	}
	dbdata->respos = 0;
	PQclear(res);
	return TCL_OK;
	error:
		if (res != NULL) PQclear(res);
		return TCL_ERROR;
}

int dbi_Postgresql_Close(
	Tcl_Interp *interp,
	Dbi *db)
{
	dbi_Postgresql_Data *dbdata = (dbi_Postgresql_Data *)db->dbdata;
	if (dbdata->conn != NULL) PQfinish(dbdata->conn);
	dbdata->conn = NULL;
	return TCL_OK;
}

int dbi_Postgresql_Destroy(
	Dbi *db)
{
	dbi_Postgresql_Data *dbdata = (dbi_Postgresql_Data *)db->dbdata;
	if (dbdata->conn != NULL) PQfinish(dbdata->conn);
	Tcl_Free((char *)dbdata);
	return TCL_OK;
}

int dbi_Postgresql_Create(
	Tcl_Interp *interp,
	Dbi *db)
{
	dbi_Postgresql_Data *dbdata;
	dbdata = (dbi_Postgresql_Data *)Tcl_Alloc(sizeof(dbi_Postgresql_Data));
	dbdata->conn = NULL;
	dbdata->res = NULL;
	db->dbdata = (ClientData)dbdata;
	db->open = dbi_Postgresql_Open;
/*	db->admin = dbi_Postgresql_Admin;*/
/*	db->configure = dbi_Postgresql_Configure;*/
	db->exec = dbi_Postgresql_Exec;
	db->tables = dbi_Postgresql_Tables;
	db->tableinfo = dbi_Postgresql_Tableinfo;
	db->fetch = dbi_Postgresql_Fetch;
	db->transaction = dbi_Postgresql_Transaction;
	db->close = dbi_Postgresql_Close;
	db->destroy = dbi_Postgresql_Destroy;
	return TCL_OK;
}

int Dbi_postgresql_Init(interp)
	Tcl_Interp *interp;		/* Interpreter to add extra commands */
{
#ifdef USE_TCL_STUBS
	if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
		return TCL_ERROR;
	}
#endif
	dbi_CreateType(interp,"postgresql",dbi_Postgresql_Create);
	Tcl_Eval(interp,"");
	return TCL_OK;
}
