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
#include "tcl.h"
#include "dbi.h"
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
	if (objc < 2) {
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
		Tcl_AppendToObj(conninfo,"' ",1);
		string = Tcl_GetStringFromObj(objv[i],&len);
		Tcl_AppendToObj(conninfo,string+1,len-1);
		Tcl_AppendToObj(conninfo,"='",1);
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
	Tcl_Obj *nullvalue)
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
			Tcl_AppendResult(interp,"error executing command \"",
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
			Tcl_AppendResult(interp,"database error while executing command \"",
				Tcl_GetStringFromObj(cmd,NULL), "\":\n",
				PQresultErrorMessage(res), NULL);
			goto error;
	}
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
	char buffer[20];
	int error,stringlen;
	int ntuples = PQntuples(res);
	int nfields = PQnfields(res);
	if (res == NULL) {
		Tcl_AppendResult(interp, "no result available: invoke exec method with -usefetch option first", NULL);
		return TCL_ERROR;
	}
	if (t >= ntuples) {
		sprintf(buffer,"%d",t);
		Tcl_AppendResult(interp, "line ",buffer ," out of range", NULL);
		return TCL_ERROR;
	}
	if (f >= nfields) {
		sprintf(buffer,"%d",f);
		Tcl_AppendResult(interp, "field ",buffer ," out of range", NULL);
		return TCL_ERROR;
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
	}
	return TCL_OK;
	error:
		if (result != NULL) Tcl_DecrRefCount(result);
		if (line != NULL) Tcl_DecrRefCount(line);
		if (element != NULL) Tcl_DecrRefCount(element);
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
	db->fetch = dbi_Postgresql_Fetch;
	db->close = dbi_Postgresql_Close;
	db->destroy = dbi_Postgresql_Destroy;
	return TCL_OK;
}

