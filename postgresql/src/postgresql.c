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

void Tcl_GetCommandFullName(
    Tcl_Interp *interp,
    Tcl_Command command,
    Tcl_Obj *objPtr);

int Dbi_Postgresql_Clone(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata,
	Tcl_Obj *name);

/******************************************************************/

void dbi_Postgresql_NoticeProcessor(
	void *arg,
	const char *message)
{
}

int dbi_Postgresql_Open(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
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

int dbi_Postgresql_GetOne(
	Tcl_Interp *interp,
	PGresult *res,
	int ituple,
	int ifield,
	Tcl_Obj *nullvalue,
	Tcl_Obj **result)
{
	char *string;
	double d;
	int len,type;
	if (PQgetisnull(res,ituple,ifield)) {
		*result = nullvalue;
	} else {
		string = PQgetvalue(res,ituple,ifield);
		len = strlen(string);
		type = PQftype(res,ifield);
		if (type == BPCHAROID) {
			while (string[len-1] == ' ') {if (len <= 0) break;len--;}
		} else if ((type == TIMEOID) || (type == TIMESTAMPOID)) {
			if (string[len-3] == '+') {len -= 3;}
		}
		*result = Tcl_NewStringObj(string,len);
		if ((type == FLOAT4OID) || (type == FLOAT8OID)) {
			/* so that the result is always given in a double format (20.0) */
			Tcl_GetDoubleFromObj(interp,*result,&d);
			Tcl_SetDoubleObj(*result, d);
		} else if ((type == TIMEOID) || (type == TIMESTAMPOID)) {
			Tcl_AppendToObj(*result,".000",4);
		}
	}
	return TCL_OK;
}

int dbi_Postgresql_ToResult(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata,
	PGresult *res,
	Tcl_Obj *nullvalue,
	int flat)
{
	Tcl_Obj *result = NULL, *line = NULL, *element = NULL;
	int t,f,error;
	int ntuples = PQntuples(res);
	int nfields = PQnfields(res);
	if (nullvalue == NULL) {
		nullvalue = dbdata->defnullvalue;
	}
	result = Tcl_NewListObj(0,NULL);
	for (t = 0 ; t < ntuples ; t++) {
		if (!flat) {line = Tcl_NewListObj(0,NULL);}
		for (f = 0 ; f < nfields ; f++) {
			dbi_Postgresql_GetOne(interp,res,t,f,nullvalue,&element);
			if (flat) {
				error = Tcl_ListObjAppendElement(interp,result,element);
			} else {
				error = Tcl_ListObjAppendElement(interp,line,element);
			}
			if (error) goto error;
			element = NULL;
		}
		if (!flat) {
			error = Tcl_ListObjAppendElement(interp,result, line);
			if (error) goto error;
			line = NULL;
		}
	}
	Tcl_SetObjResult(interp, result);
	return TCL_OK;
	error:
		if (result != NULL) Tcl_DecrRefCount(result);
		if (line != NULL) Tcl_DecrRefCount(line);
		if (element != NULL) Tcl_DecrRefCount(element);
		return TCL_ERROR;
}


int dbi_Postgresql_Error(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata)
{
	Tcl_AppendResult(interp,"database error:\n",
		PQerrorMessage(dbdata->conn), NULL);
	return TCL_ERROR;
}

int dbi_Postgresql_ParseStatement(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata,
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

int dbi_Postgresql_Exec(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata,
	Tcl_Obj *cmd,
	int flags,
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	PGresult *res = NULL;
	ExecStatusType status;
	char *cmdstring,*nullstring;
	int *parsedstatement;
	int error,numargs,len,nulllen,usefetch,flat;
	flat = flags & EXEC_FLAT;
	usefetch = flags & EXEC_USEFETCH;
	if (dbdata->conn == NULL) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		goto error;
	}
	cmdstring = Tcl_GetStringFromObj(cmd,&len);
	error = dbi_Postgresql_ParseStatement(interp,dbdata,cmdstring,len,&parsedstatement,&numargs);
	if (objc != numargs) {
		Tcl_Free((char *)parsedstatement);
		Tcl_AppendResult(interp,"wrong number of arguments given to exec", NULL);
		Tcl_AppendResult(interp," while executing command: \"",	Tcl_GetStringFromObj(cmd,NULL), "\"", NULL);
		return TCL_ERROR;
	}
	if (nullvalue == NULL) {
		nullstring = NULL; nulllen = 0;
	} else {
		nullstring = Tcl_GetStringFromObj(nullvalue,&nulllen);
	}
	if (numargs > 0) {
		char *sqlstring,*cursqlstring,*argstring;
		double tempd;
		int i,size,prvsrcpos,arglen,tempi;
		/* calculate size of buffer needed */
		size = len;
		for (i = 0 ; i < numargs ; i++) {
			Tcl_GetStringFromObj(objv[i],&arglen);
			/* we need at least space for 4 characters: in case of a NULL */
			if (arglen < 2) {arglen = 2;}
			size += arglen+2;
		}
		sqlstring = (char *)Tcl_Alloc(size*sizeof(char));
		cursqlstring = sqlstring;
		prvsrcpos = 0;
		/* put sql with inserted parameters into buffer */
		for (i = 0 ; i < numargs ; i++) {
			strncpy(cursqlstring,cmdstring+prvsrcpos,parsedstatement[i]-prvsrcpos);
			cursqlstring += parsedstatement[i]-prvsrcpos;
			prvsrcpos = parsedstatement[i]+1;
			argstring = Tcl_GetStringFromObj(objv[i],&arglen);
			if ((nullstring != NULL)&&(arglen == nulllen)&&(strncmp(argstring,nullstring,arglen) == 0)) {
				strncpy(cursqlstring,"NULL",4);
				cursqlstring += 4;
			} else if ((Tcl_GetIntFromObj(interp,objv[i],&tempi) == TCL_OK) || (Tcl_GetDoubleFromObj(interp,objv[i],&tempd) == TCL_OK)) {
				strncpy(cursqlstring,argstring,arglen);
				cursqlstring += arglen;
			} else {
				*cursqlstring++ = '\'';
				strncpy(cursqlstring,argstring,arglen);
				cursqlstring += arglen;
				*cursqlstring++ = '\'';
			}
		}
		Tcl_ResetResult(interp);
		strncpy(cursqlstring,cmdstring+prvsrcpos,len-prvsrcpos);
		cursqlstring[len-prvsrcpos] = '\0';
		Tcl_Free((char *)parsedstatement);
		res = PQexec(dbdata->conn, sqlstring);
		Tcl_Free(sqlstring);
	} else {
		res = PQexec(dbdata->conn, Tcl_GetStringFromObj(cmd,NULL));
	}
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
				error = dbi_Postgresql_ToResult(interp,dbdata,res,nullvalue,flat);
				if (error) {goto error;}
			}
			break;
		default:
			Tcl_AppendResult(interp,"database error: \"",PQresultErrorMessage(res), NULL);
			Tcl_AppendResult(interp," while executing command: \"",	Tcl_GetStringFromObj(cmd,NULL), "\"", NULL);
			goto error;
	}
	dbdata->respos = -1;
	if (usefetch) {
		if (dbdata->res != NULL) {
			PQclear(dbdata->res);
		}
		dbdata->res = res;
	} else {
		PQclear(res);
		dbdata->res = NULL;
	}
	return TCL_OK;
	error:
		if (res != NULL) PQclear(res);
		return TCL_ERROR;
}

int dbi_Postgresql_Fetch(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata,
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
	int ituple = -1,ifield = -1;
	PGresult *res = dbdata->res;
	Tcl_Obj *result = NULL, *line = NULL, *element = NULL;
	char *string;
	int error;
	int ntuples = PQntuples(res);
	int nfields = PQnfields(res);
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
				ituple = dbdata->respos;
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
	 * fetch data
	 * ----------
	 */
	if (res == NULL) {
		Tcl_AppendResult(interp, "no result available: invoke exec method with -usefetch option first", NULL);
		return TCL_ERROR;
	}
	switch (fetch_option) {
		case Lines:
			Tcl_SetObjResult(interp,Tcl_NewIntObj(ntuples));
			return TCL_OK;
		case Fields:
			line = Tcl_NewListObj(0,NULL);
			for (ifield = 0 ; ifield < nfields ; ifield++) {
				string = PQfname(res,ifield);
				element = Tcl_NewStringObj(string,strlen(string));
				error = Tcl_ListObjAppendElement(interp,line, element);
				if (error) goto error;
				element = NULL;
			}
			Tcl_SetObjResult(interp, line);
			line = NULL;
			return TCL_OK;
		case Clear:
			PQclear(res);
			dbdata->res = NULL;
			return TCL_OK;
		case Pos:
			Tcl_SetObjResult(interp,Tcl_NewIntObj(dbdata->respos));
			return TCL_OK;
	}
	if (ituple != -1) {
		dbdata->respos = ituple;
	} else {
		dbdata->respos++;
	}
	if (dbdata->respos >= ntuples) {
		Tcl_Obj *buffer;
		buffer = Tcl_NewIntObj(ntuples);
		Tcl_AppendResult(interp, "line ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
		Tcl_DecrRefCount(buffer);
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
			if (ifield == -1) {
				line = Tcl_NewListObj(0,NULL);
				for (ifield = 0 ; ifield < nfields ; ifield++) {
					dbi_Postgresql_GetOne(interp,res,dbdata->respos,ifield,nullvalue,&element);
					error = Tcl_ListObjAppendElement(interp,line, element);
					if (error) {Tcl_DecrRefCount(nullvalue);goto error;}
					element = NULL;
				}
				Tcl_SetObjResult(interp, line);
				line = NULL;
			} else {
				dbi_Postgresql_GetOne(interp,res,dbdata->respos,ifield,nullvalue,&element);
				Tcl_SetObjResult(interp, element);
				line = NULL;
			}
			break;
		case Isnull:
			Tcl_SetObjResult(interp,Tcl_NewIntObj(PQgetisnull(res,dbdata->respos,ifield)));
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
	dbi_Postgresql_Data *dbdata)
{
	int error;
	Tcl_Obj *cmd = Tcl_NewStringObj("select relname from pg_class where (relkind = 'r' or relkind = 'v') and relname !~ '^pg_' and relname !~ '^pga_'",-1);
	error = dbi_Postgresql_Exec(interp,dbdata,cmd,EXEC_FLAT,NULL,0,(Tcl_Obj **)NULL);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Postgresql_Transaction_Start(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata)
{
	int error;
	Tcl_Obj *cmd = Tcl_NewStringObj("begin",5);
	error = dbi_Postgresql_Exec(interp,dbdata,cmd,EXEC_FLAT,NULL,0,(Tcl_Obj **)NULL);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Postgresql_Transaction_Commit(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata)
{
	int error;
	Tcl_Obj *cmd = Tcl_NewStringObj("commit",6);
	error = dbi_Postgresql_Exec(interp,dbdata,cmd,EXEC_FLAT,NULL,0,(Tcl_Obj **)NULL);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Postgresql_Transaction_Rollback(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata)
{
	int error;
	Tcl_Obj *cmd = Tcl_NewStringObj("rollback",8);
	error = dbi_Postgresql_Exec(interp,dbdata,cmd,EXEC_FLAT,NULL,0,(Tcl_Obj **)NULL);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Postgresql_Serial(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata,
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
			cmd = Tcl_NewStringObj("::dbi::postgresql::serial_add",-1);
			break;
		case Delete:
			cmd = Tcl_NewStringObj("::dbi::postgresql::serial_delete",-1);
			current = NULL;
			break;
		case Set:
			cmd = Tcl_NewStringObj("::dbi::postgresql::serial_set",-1);
			break;
		case Next:
			cmd = Tcl_NewStringObj("::dbi::postgresql::serial_next",-1);
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

int dbi_Postgresql_Supports(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata,
	Tcl_Obj *keyword)
{
	static char *keywords[] = {
		"lines","backfetch","transactions","sharedtransactions","foreignkeys","checks","permissions",
		(char *) NULL};
	enum keywordsIdx {
		Lines, Backfetch,Transactions,Sharedtransactions,Foreignkeys,Checks,Permissions
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

int dbi_Postgresql_Info(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata,
	int objc,
	Tcl_Obj **objv)
{
	Tcl_Obj *cmd,*dbcmd;
	int error,i;
	dbcmd = Tcl_NewObj();
	Tcl_GetCommandFullName(interp, dbdata->token, dbcmd);
	cmd = Tcl_NewStringObj("::dbi::postgresql::info",-1);
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

int dbi_Postgresql_Cmd(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata,
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

int dbi_Postgresql_Interface(
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
		char *interface;
		int len;
		interface = Tcl_GetStringFromObj(objv[2],&len);
		i = 0;
		while (interfaces[i] != NULL) {
			if ((strlen(interfaces[i]) == len) && (strncmp(interfaces[i],interface,len) == 0)) {
				Tcl_AppendResult(interp,interfaces[i+1],NULL);
				return TCL_OK;
			}
			i+=2;
		}
		Tcl_AppendResult(interp, Tcl_GetStringFromObj(objv[0],NULL), 
			" does not support interface ", interface, NULL);
		return TCL_ERROR;
	}
}

int dbi_Postgresql_Close(
	dbi_Postgresql_Data *dbdata)
{
	while (dbdata->clonesnum) {
		Tcl_DeleteCommandFromToken(dbdata->interp,dbdata->clones[0]->token);
	}
	if (dbdata->clones != NULL) {
		Tcl_Free((char *)dbdata->clones);
		dbdata->clones = NULL;
	}
	if (dbdata->parent == NULL) {
		if (dbdata->conn != NULL) PQfinish(dbdata->conn);
		dbdata->conn = NULL;
	} else {
		dbi_Postgresql_Data *parent = dbdata->parent;
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

int dbi_Postgresql_Destroy(
	ClientData clientdata)
{
	dbi_Postgresql_Data *dbdata = (dbi_Postgresql_Data *)clientdata;
	dbi_Postgresql_Close(dbdata);
	Tcl_DecrRefCount(dbdata->defnullvalue);
	Tcl_Free((char *)dbdata);
	Tcl_DeleteExitHandler((Tcl_ExitProc *)dbi_Postgresql_Destroy, clientdata);
	return TCL_OK;
}

static int dbi_num = 0;
int Dbi_Postgresql_DbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_Postgresql_Data *dbdata = (dbi_Postgresql_Data *)clientdata;
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
		return dbi_Postgresql_Interface(interp,objc,objv);
    case Open:
		if (dbdata->parent != NULL) {
			Tcl_AppendResult(interp,"clone may not use open",NULL);
			return TCL_ERROR;
		}
		return dbi_Postgresql_Open(interp,dbdata,objc,objv);
	case Destroy:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		Tcl_DeleteCommandFromToken(interp,dbdata->token);
		return TCL_OK;
	case Create:
		Tcl_AppendResult(interp,"dbi_postgresql does not support createdb\n", NULL);
		return TCL_ERROR;
/*		return dbi_Postgresql_Createdb(interp,dbdata,objc,objv);*/
	case Drop:
		Tcl_AppendResult(interp,"dbi_postgresql does not support dropdb\n", NULL);
/*		return TCL_ERROR;
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_Postgresql_Dropdb(interp,dbdata);
*/
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
				if (strncmp(Tcl_GetStringFromObj(dbcmd,NULL),"::dbi::postgresql::priv_",23) == 0) continue;
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
	if (dbdata->conn == NULL) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		return TCL_ERROR;
	}
    switch (index) {
	case Exec:
		{
	    static char *switches[] = {"-usefetch", "-nullvalue", "-flat", "-cache",(char *) NULL};
	    enum switchesIdx {Usefetch, Nullvalue, Flat, Cache};
		Tcl_Obj *nullvalue = NULL;
		char *string;
		int flags = 0;
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
		return dbi_Postgresql_Exec(interp,dbdata,objv[i],flags,nullvalue,objc-i-1,objv+i+1);
		}
	case Fetch:
		return dbi_Postgresql_Fetch(interp,dbdata, objc, objv);
	case Info:
		return dbi_Postgresql_Info(interp,dbdata,objc-2,objv+2);
	case Tables:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_Postgresql_Tables(interp,dbdata);
	case Fields:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "fields tablename");
			return TCL_ERROR;
		}
		return dbi_Postgresql_Cmd(interp,dbdata,objv[2],NULL,"::dbi::postgresql::fieldsinfo");
	case Close:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->parent == NULL) {
			return dbi_Postgresql_Close(dbdata);
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
		if (dbdata->conn == NULL) {
			Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
			return error;
		}
		error = dbi_Postgresql_Transaction_Start(interp,dbdata);
		if (error) {return dbi_Postgresql_Error(interp,dbdata);}
		return TCL_OK;
	case Commit:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->conn == NULL) {
			Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
			return error;
		}
		error = dbi_Postgresql_Transaction_Commit(interp,dbdata);
		if (error) {return dbi_Postgresql_Error(interp,dbdata);}
		return TCL_OK;
	case Rollback:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->conn == NULL) {
			Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
			return error;
		}
		error = dbi_Postgresql_Transaction_Rollback(interp,dbdata);
		if (error) {return dbi_Postgresql_Error(interp,dbdata);}
		return TCL_OK;
	case Serial:
		if (objc < 5) {
			Tcl_WrongNumArgs(interp, 1, objv, "serial option table field ?value?");
			return TCL_ERROR;
		}
		if (objc == 6) {
			return dbi_Postgresql_Serial(interp,dbdata,objv[2],objv[3],objv[4],objv[5]);
		} else {
			return dbi_Postgresql_Serial(interp,dbdata,objv[2],objv[3],objv[4],NULL);
		}
	case Clone:
		if ((objc != 2) && (objc != 3)) {
			Tcl_WrongNumArgs(interp, 2, objv, "?name?");
			return TCL_ERROR;
		}
		if (objc == 3) {
			return Dbi_Postgresql_Clone(interp,dbdata,objv[2]);
		} else {
			return Dbi_Postgresql_Clone(interp,dbdata,NULL);
		}
	case Supports:
		if (objc == 2) {
			return dbi_Postgresql_Supports(interp,dbdata,NULL);
		} if (objc == 3) {
			return dbi_Postgresql_Supports(interp,dbdata,objv[2]);
		} else {
			Tcl_WrongNumArgs(interp, 2, objv, "?keyword?");
			return TCL_ERROR;
		}
	}
	return error;
}

int Dbi_Postgresql_DoNewDbObjCmd(
	dbi_Postgresql_Data *dbdata,
	Tcl_Interp *interp,
	Tcl_Obj *dbi_nameObj)
{
	char buffer[40];
	char *dbi_name;
	dbdata->conn = NULL;
	dbdata->respos = -1;
	dbdata->res = NULL;
	dbdata->clones = NULL;
	dbdata->clonesnum = 0;
	dbdata->parent = NULL;
	if (dbi_nameObj == NULL) {
		dbi_num++;
		sprintf(buffer,"::dbi::postgresql::dbi%d",dbi_num);
		dbi_nameObj = Tcl_NewStringObj(buffer,strlen(buffer));
	}
	dbi_name = Tcl_GetStringFromObj(dbi_nameObj,NULL);
	dbdata->interp = interp;
	dbdata->defnullvalue = Tcl_NewObj();
	Tcl_IncrRefCount(dbdata->defnullvalue);
	dbdata->token = Tcl_CreateObjCommand(interp,dbi_name,(Tcl_ObjCmdProc *)Dbi_Postgresql_DbObjCmd,
		(ClientData)dbdata,(Tcl_CmdDeleteProc *)dbi_Postgresql_Destroy);
	Tcl_CreateExitHandler((Tcl_ExitProc *)dbi_Postgresql_Destroy, (ClientData)dbdata);
	Tcl_SetObjResult(interp,dbi_nameObj);
	return TCL_OK;
}

int Dbi_Postgresql_NewDbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_Postgresql_Data *dbdata;
	int error;
	if ((objc < 1)||(objc > 2)) {
		Tcl_WrongNumArgs(interp,2,objv,"?dbName?");
		return TCL_ERROR;
	}
	dbdata = (dbi_Postgresql_Data *)Tcl_Alloc(sizeof(dbi_Postgresql_Data));
	if (objc == 2) {
		error = Dbi_Postgresql_DoNewDbObjCmd(dbdata,interp,objv[1]);
	} else {
		error = Dbi_Postgresql_DoNewDbObjCmd(dbdata,interp,NULL);
	}
	if (error) {
		Tcl_Free((char *)dbdata);
	}
	return error;
}

int Dbi_Postgresql_Clone(
	Tcl_Interp *interp,
	dbi_Postgresql_Data *dbdata,
	Tcl_Obj *name)
{
	dbi_Postgresql_Data *parent = NULL;
	dbi_Postgresql_Data *clone_dbdata = NULL;
	int error;
	parent = dbdata;
	while (parent->parent != NULL) {parent = parent->parent;}
	clone_dbdata = (dbi_Postgresql_Data *)Tcl_Alloc(sizeof(dbi_Postgresql_Data));
	error = Dbi_Postgresql_DoNewDbObjCmd(clone_dbdata,interp,name);
	if (error) {Tcl_Free((char *)clone_dbdata);return TCL_ERROR;}
	name = Tcl_GetObjResult(interp);
	parent->clonesnum++;
	parent->clones = (dbi_Postgresql_Data **)Tcl_Realloc((char *)parent->clones,parent->clonesnum*sizeof(dbi_Postgresql_Data **));
	parent->clones[parent->clonesnum-1] = clone_dbdata;
	clone_dbdata->parent = parent;
	clone_dbdata->conn = parent->conn;
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
	Tcl_CreateObjCommand(interp,"dbi_postgresql",(Tcl_ObjCmdProc *)Dbi_Postgresql_NewDbObjCmd,
		(ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
	Tcl_Eval(interp,"");
	return TCL_OK;
}
