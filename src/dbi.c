/*
 *       File:    dbi.c
 *       Purpose: dbi extension to Tcl
 *       Author:  Copyright (c) 1998 Peter De Rijk
 *
 *       See the file "README" for information on usage and redistribution
 *       of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "tcl.h"
#include "dbi.h"

/* #ifdef unix*/
#define export
/* #endif */

int dbi_PostgresqlInit(Tcl_Interp *interp,Dbi *db,int objc,Tcl_Obj *objv[]);

Tcl_HashTable *dstypesTable = NULL;

int dbi_DbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	Dbi *db = (Dbi *)clientdata;
	char *cmd;
	int error=TCL_OK,cmdlen,i;
	if (objc < 2) {
		Tcl_WrongNumArgs(interp, 1, objv, "option ?...?");
		return TCL_ERROR;
	}
	cmd = Tcl_GetStringFromObj(objv[1],&cmdlen);
	if ((cmdlen == 4)&&(strncmp(cmd,"open",4) == 0)) {
		error = db->open(interp,db,objc,objv);
		if (error) {return TCL_ERROR;}
		return TCL_OK;
	} else	if ((cmdlen == 4)&&(strncmp(cmd,"exec",4) == 0)) {
		Tcl_Obj *temp, *nullvalue = NULL, *command = NULL;
		char *string;
		int usefetch = 0;
		int objn,stringlen;
		if (objc < 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "?options? command ?argument? ?...?");
			return TCL_ERROR;
		}
		i = 2;
		while (i < objc) {
			string = Tcl_GetStringFromObj(objv[i],&stringlen);
			if (string[0] != '-') break;
			if ((stringlen==9)&&(strncmp(string,"-usefetch",9)==0)) {
				if (db->fetch == NULL) {
					Tcl_AppendResult(interp,db->type, " type: fetch not supported",NULL);
					return TCL_ERROR;
				}
				usefetch = 1;
			} else if ((stringlen==8)&&(strncmp(string,"-command",8)==0)) {
				i++;
				if (i == objc) {
					Tcl_AppendResult(interp,"no value given for option \"-command\"",NULL);
					return TCL_ERROR;
				}
				Tcl_AppendResult(interp,db->type, " type: -command not supported",NULL);
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
		error = db->exec(interp,db,objv[i],usefetch,nullvalue,objc-i-1,objv+i+1);
		if (error) {return TCL_ERROR;}
		return TCL_OK;
	} else if ((cmdlen == 5)&&(strncmp(cmd,"fetch",5) == 0)) {
		Tcl_Obj *tuple = NULL, *field = NULL, *nullvalue = NULL;
		char *string;
		int i,stringlen,fetch_option = DBI_FETCH_DATA,t,f;
		if (db->fetch == NULL) {
			Tcl_AppendResult(interp,db->type, " type: fetch not supported",NULL);
			return TCL_ERROR;
		}
		if (objc < 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "?options? ?line? ?field?");
			return TCL_ERROR;
		}
		i = 2;
		while (i < objc) {
			string = Tcl_GetStringFromObj(objv[i],&stringlen);
			if (string[0] != '-') break;
			if ((stringlen==10)&&(strncmp(string,"-nullvalue",10)==0)) {
				i++;
				if (i == objc) {
					Tcl_AppendResult(interp,"no value given for option \"-nullvalue\"",NULL);
					return TCL_ERROR;
				}
				nullvalue = objv[i];
			} else if ((stringlen==6)&&(strncmp(string,"-lines",6)==0)) {
				fetch_option = DBI_FETCH_LINES;
			} else if ((stringlen==8)&&(strncmp(string,"-current",8)==0)) {
				fetch_option = DBI_FETCH_CURRENTLINE;
			} else if ((stringlen==7)&&(strncmp(string,"-fields",7)==0)) {
				fetch_option = DBI_FETCH_FIELDS;
			} else if ((stringlen==6)&&(strncmp(string,"-clear",6)==0)) {
				fetch_option = DBI_FETCH_CLEAR;
			} else if ((stringlen==7)&&(strncmp(string,"-isnull",7)==0)) {
				fetch_option = DBI_FETCH_ISNULL;
			} else {
				Tcl_AppendResult(interp,"unknown option \"",string,"\", must be one of: -nullvalue, -lines, -fields, -clear, -isnull",NULL);
				return TCL_ERROR;
			}
			i++;
		}
		if (i == objc) {
		} else if (i == (objc-1)) {
			tuple = objv[i];
		} else if (i == (objc-2)) {
			tuple = objv[i];
			field = objv[i+1];
		} else {
			Tcl_WrongNumArgs(interp, 2, objv, "?options? ?line? ?field?");
			return TCL_ERROR;
		}
		if (tuple != NULL) {
			error = Tcl_GetIntFromObj(interp,tuple,&t);
			if (t < 0) {
				Tcl_Obj *buffer;
				buffer = Tcl_NewIntObj(t);
				Tcl_AppendResult(interp, "line ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
				Tcl_DecrRefCount(buffer);
				return TCL_ERROR;
			}
			if (error) {return TCL_ERROR;}
		} else {
			t = -1;
		}
		if (field != NULL) {
			error = Tcl_GetIntFromObj(interp,field,&f);
			if (error) {return TCL_ERROR;}
		} else {
			f = -1;
		}
		error = db->fetch(interp,db,fetch_option,t,f,nullvalue);
		if (error) {return TCL_ERROR;}
		return TCL_OK;
	} else if ((cmdlen == 5)&&(strncmp(cmd,"admin",5) == 0)) {
		if (db->admin == NULL) {
			Tcl_AppendResult(interp,db->type, "type: admin functions not supported",NULL);
			return TCL_ERROR;
		}
		error = db->admin(interp,db,objc,objv);
		if (error) {return TCL_ERROR;}
		return TCL_OK;
	} else if ((cmdlen == 9)&&(strncmp(cmd,"configure",9) == 0)) {
		if (db->admin == NULL) {
			Tcl_AppendResult(interp,db->type, "type: configuration options not supported",NULL);
			return TCL_ERROR;
		}
		if (objc == 2) {
			error = db->configure(interp,db,NULL,NULL);
			if (error) {return TCL_ERROR;}
			return TCL_OK;
		} else if (objc == 3) {
			error = db->configure(interp,db,objv[1],NULL);
		} else {
			if (objc%2 != 0) {
				Tcl_WrongNumArgs(interp, 2, objv, "?option? ?value? ?option value? ...");
				return TCL_ERROR;
			}
			for (i = 0 ; i < objc ; i += 2) {
				error = db->configure(interp,db,objv[i],objv[i+1]);
				if (error) {return TCL_ERROR;}
				return TCL_OK;
			}
		}
		if (error) {return TCL_ERROR;}
		return TCL_OK;
	} else if ((cmdlen == 6)&&(strncmp(cmd,"tables",6) == 0)) {
		if (db->tables == NULL) {
			Tcl_AppendResult(interp,db->type, " type: tables not supported",NULL);
			return TCL_ERROR;
		}
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		error = db->tables(interp,db);
		if (error) {return TCL_ERROR;}
		return TCL_OK;
	} else if ((cmdlen == 9)&&(strncmp(cmd,"tableinfo",9) == 0)) {
		if (db->tableinfo == NULL) {
			Tcl_AppendResult(interp,db->type, " type: tableinfo not supported",NULL);
			return TCL_ERROR;
		}
		if (objc != 4) {
			Tcl_WrongNumArgs(interp, 2, objv, "tablename varName");
			return TCL_ERROR;
		}
		error = db->tableinfo(interp,db,objv[2],objv[3]);
		if (error) {return TCL_ERROR;}
		return TCL_OK;
	} else if ((cmdlen == 5)&&(strncmp(cmd,"close",5) == 0)) {
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		error = db->close(interp,db);
		if (error) {return TCL_ERROR;}
		return TCL_OK;
	} else if ((cmdlen == 5)&&(strncmp(cmd,"begin",5) == 0)) {
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (db->transaction == NULL) {
			Tcl_AppendResult(interp,db->type, " type: transactions not supported",NULL);
			return TCL_ERROR;
		}
		error = db->transaction(interp,db,TRANSACTION_BEGIN);
		if (error) {return TCL_ERROR;}
		return TCL_OK;
	} else if ((cmdlen == 6)&&(strncmp(cmd,"commit",6) == 0)) {
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (db->transaction == NULL) {
			Tcl_AppendResult(interp,db->type, " type: transactions not supported",NULL);
			return TCL_ERROR;
		}
		error = db->transaction(interp,db,TRANSACTION_COMMIT);
		if (error) {return TCL_ERROR;}
		return TCL_OK;
	} else if ((cmdlen == 8)&&(strncmp(cmd,"rollback",8) == 0)) {
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (db->transaction == NULL) {
			Tcl_AppendResult(interp,db->type, " type: transactions not supported",NULL);
			return TCL_ERROR;
		}
		error = db->transaction(interp,db,TRANSACTION_ROLLBACK);
		if (error) {return TCL_ERROR;}
		return TCL_OK;
	} else if ((cmdlen == 7)&&(strncmp(cmd,"destroy",7) == 0)) {
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		Tcl_DeleteCommandFromToken(interp,db->token);
		return TCL_OK;
	} else if ((cmdlen == 6)&&(strncmp(cmd,"serial",6) == 0)) {
		Tcl_Obj *current = NULL;
		int type;
		if (db->serial == NULL) {
			Tcl_AppendResult(interp,db->type, " type: serial option not supported",NULL);
			return TCL_ERROR;
		}
		if ((objc < 5)||(objc > 6)) {
			Tcl_WrongNumArgs(interp, 2, objv, "cmd table field ?current?");
			return TCL_ERROR;
		}
		cmd = Tcl_GetStringFromObj(objv[2],&cmdlen);
		if ((cmdlen == 3)&&(strncmp(cmd,"add",3) == 0)) {
			type = SERIAL_ADD;
		} else	if ((cmdlen == 6)&&(strncmp(cmd,"delete",6) == 0)) {
			type = SERIAL_DELETE;
		} else	if ((cmdlen == 3)&&(strncmp(cmd,"set",3) == 0)) {
			type = SERIAL_SET;
		} else {
			Tcl_ResetResult(interp);
			Tcl_AppendResult(interp,"bad suboption \"", cmd, "\" for serial: must be one of add, delete or set", (char *)NULL);
			return TCL_ERROR;
		}
		if (objc == 6) {
			current = objv[5];
		}
		error = db->serial(interp,db,type,objv[3],objv[4],current);
		if (error) {return TCL_ERROR;}
		return TCL_OK;
	}
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp,"bad option \"", cmd, "\": must be one of open, configure, exec, fetch, tables, tableinfo, serial, close", (char *)NULL);
	return TCL_ERROR;
}

export int dbi_CreateType(
	Tcl_Interp *interp,
	char *type,
	dbi_TypeCreate (*create))
{
	Tcl_HashEntry *entry;
	int new;
	if (dstypesTable == NULL) {
		dstypesTable = (Tcl_HashTable *)Tcl_Alloc(sizeof(Tcl_HashTable));
		Tcl_InitHashTable(dstypesTable,TCL_STRING_KEYS);
	}
	entry = Tcl_CreateHashEntry(dstypesTable,type,&new);
	if (new == 1) {
		Tcl_SetHashValue(entry,(ClientData)create);
	} else {
		Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, "type \"", type, "\" already exists", (char *) NULL);
		return TCL_ERROR;
	}
	return TCL_OK;
}

void dbi_DbDestroy(ClientData clientdata) {
	Dbi *db;
	db = (Dbi *)clientdata;
	(db->destroy)(db);
	Tcl_DeleteExitHandler((Tcl_ExitProc *)dbi_DbDestroy, clientdata);
	Tcl_Free((char *)db);
}

static int dbi_num = 0;

int dbi_NewDbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	Dbi *db;
	Tcl_HashSearch search;
	Tcl_HashEntry *entry;
	Tcl_Obj *dbi_nameObj = NULL;
	char buffer[20];
	char *dbi_name;
	char *type,*string;
	int typelen,stringlen,error,start;
	dbi_TypeCreate (*createf);
	if (objc < 2) {
		Tcl_AppendResult(interp,"wrong # args: should be \"",
			Tcl_GetStringFromObj(objv[0],NULL), " type ?dbName?\" or \"",
			Tcl_GetStringFromObj(objv[0],NULL), " info ...\"",NULL);
		return TCL_ERROR;
	}
	type = Tcl_GetStringFromObj(objv[1],&typelen);
	if ((typelen == 4)&&(strncmp(type,"info",4)==0)) {
		if (objc != 3) {
			Tcl_WrongNumArgs(interp,1,objv,"info item");
			return TCL_ERROR;
		}
		string = Tcl_GetStringFromObj(objv[2],&stringlen);
		if ((stringlen == 11)&&(strncmp(string,"typesloaded",11)==0)) {
			Tcl_ResetResult(interp);
		if (dstypesTable == NULL) {return TCL_OK;}
			entry = Tcl_FirstHashEntry(dstypesTable, &search);
			while(1) {
				if (entry == NULL) break;
				type = Tcl_GetHashKey(dstypesTable, entry);
				Tcl_AppendElement(interp,type);
				entry = Tcl_NextHashEntry(&search);
			}
		} else {
			error = Tcl_VarEval(interp, "::dbi::info ",string,NULL);
			if (error) {return error;}
		}
		return TCL_OK;
	} else {
		type = Tcl_GetStringFromObj(objv[1],&typelen);
		error = Tcl_VarEval(interp, "::dbi::load ",type,NULL);
		if (error) {return error;}
		entry = Tcl_FindHashEntry(dstypesTable,type);
		if (entry == NULL) {
			Tcl_ResetResult(interp);
			Tcl_AppendResult(interp, "Unknown dbi type: ", type, (char *) NULL);
			return TCL_ERROR;
		}
		createf = Tcl_GetHashValue(entry);
		db = (Dbi *)Tcl_Alloc(sizeof(Dbi));
		db->type = Tcl_GetHashKey(dstypesTable,entry);
		db->dbdata = NULL;
		db->open = NULL;
		db->configure = NULL;
		db->exec = NULL;
		db->fetch = NULL;
		db->close = NULL;
		db->transaction = NULL;
		db->admin = NULL;
		db->tables = NULL;
		db->tableinfo = NULL;
		db->serial = NULL;
		error = createf(interp,db);
		if (error) {return error;}
		if (objc == 3) {
			dbi_nameObj = objv[2];
			start = 3;
		} else {
			dbi_num++;
			sprintf(buffer,"dbi::dbi%d",dbi_num);
			dbi_nameObj = Tcl_NewStringObj(buffer,strlen(buffer));
			start = 2;
		}
		dbi_name = Tcl_GetStringFromObj(dbi_nameObj,NULL);
		db->token = Tcl_CreateObjCommand(interp,dbi_name,(Tcl_ObjCmdProc *)dbi_DbObjCmd,
			(ClientData)db,(Tcl_CmdDeleteProc *)dbi_DbDestroy);
		Tcl_CreateExitHandler((Tcl_ExitProc *)dbi_DbDestroy, (ClientData)db);
		Tcl_SetObjResult(interp,dbi_nameObj);
		return TCL_OK;
	}
}

