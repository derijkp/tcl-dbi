/*
 *       File:    mem.c
 *       Purpose: dbi extension to Tcl: sqlite backend
 *       Author:  Copyright (c) 1998 Peter De Rijk
 *
 *       See the file "README" for information on usage and redistribution
 *       of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "dbi_sqlite.h"

void Tcl_GetCommandFullName(
    Tcl_Interp *interp,
    Tcl_Command command,
    Tcl_Obj *objPtr);

typedef struct vary {
	short vary_length;
	char vary_string[1];
} VARY;

int Dbi_sqlite_Clone(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
	Tcl_Obj *name);

int dbi_Sqlite_Free_Stmt(
	dbi_Sqlite_Data *dbdata,
	int action);

/******************************************************************/

int dbi_Sqlite_Error(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
	char *premsg)
{
	if (strlen(premsg) != 0) {
		Tcl_AppendResult(interp,"error ",premsg,":\n", NULL);
	}
	if (dbdata->errormsg != NULL) {
		Tcl_AppendResult(interp,"error ",dbdata->errormsg,NULL);
		free(dbdata->errormsg);
		dbdata->errormsg = NULL;
	}
	return TCL_ERROR;
}

int dbi_Sqlite_autocommit_state(
	dbi_Sqlite_Data *dbdata)
{
	if (dbdata->parent != NULL) {
		return dbdata->parent->autocommit;
	} else {
		return dbdata->autocommit;
	}
}

int dbi_Sqlite_autocommit(
	dbi_Sqlite_Data *dbdata,
	int state)
{
	if (dbdata->parent != NULL) {
		dbdata->parent->autocommit = state;
	} else {
		dbdata->autocommit = state;
	}
	return state;
}

int dbi_Sqlite_TclEval(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
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

int dbi_Sqlite_Open(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	char *errormsg;
	int error;
	if (dbdata->db != NULL) {
		Tcl_AppendResult(interp,"dbi object has open connection, close first", NULL);
		goto error;
	}
	if (objc < 3) {
		Tcl_WrongNumArgs(interp,2,objv,"database");
		return TCL_ERROR;
	}
	error = dbi_Sqlite_TclEval(interp,dbdata,"::dbi::sqlite::open_test",objc-2,objv+2);	
	if (error) {return error;}
	dbdata->database = Tcl_GetObjResult(interp);
	Tcl_IncrRefCount(dbdata->database);
	dbdata->autocommit = 1;
	dbdata->db = sqlite_open(Tcl_GetStringFromObj(dbdata->database,NULL), 0, &errormsg);
	if ((dbdata->db == NULL) || (errormsg != NULL)) {
		Tcl_AppendResult(interp,"connection to database \"",
			Tcl_GetStringFromObj(dbdata->database,NULL) , "\" failed:\n", errormsg, NULL);
		free(errormsg);
		Tcl_DecrRefCount(dbdata->database);dbdata->database=NULL;
		goto error;
	}
	return TCL_OK;
	error:
		return TCL_ERROR;
}

int dbi_Sqlite_Fetch(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	Tcl_Obj *element;
	Tcl_Obj *tuple = NULL, *field = NULL, *nullvalue = NULL;
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
	error = Tcl_ListObjGetElements(interp,dbdata->result,&(dbdata->ntuples),&(dbdata->resultlines));
	if (error) return TCL_ERROR;
	switch (fetch_option) {
		case Lines:
			Tcl_SetObjResult(interp,Tcl_NewIntObj(dbdata->ntuples));
			return TCL_OK;
		case Fields:
			Tcl_SetObjResult(interp,dbdata->resultfields);
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
		ituple = ++(dbdata->tuple);
	}
	if (ituple > dbdata->ntuples) {
		goto out_of_position;
	}
	if (ifield >= dbdata->nfields) {
		Tcl_Obj *buffer;
		buffer = Tcl_NewIntObj(ifield);
		Tcl_AppendResult(interp, "field ",Tcl_GetStringFromObj(buffer,NULL) ," out of range", NULL);
		Tcl_DecrRefCount(buffer);
		return TCL_ERROR;
	}
	if (ituple >= dbdata->ntuples) {
		goto out_of_position;
	}
	if (ituple != dbdata->tuple) {dbdata->tuple = ituple;}
	if (nullvalue == NULL) {
		nullvalue = dbdata->defnullvalue;
	}
	switch (fetch_option) {
		case Data:
			if (ifield == -1) {
				if (nullvalue == dbdata->defnullvalue) {
					Tcl_SetObjResult(interp, dbdata->resultlines[ituple]);
				} else {
					Tcl_Obj *line,**valuev;
					int valuec,i;
					error = Tcl_ListObjGetElements(interp,dbdata->resultlines[ituple],&valuec,&valuev);
					if (error) {return TCL_ERROR;}
					line = Tcl_NewObj();
					for(i = 0 ; i < valuec ; i++) {
						if (valuev[i] == dbdata->defnullvalue) {
							error = Tcl_ListObjAppendElement(interp,line,nullvalue);
						} else {
							error = Tcl_ListObjAppendElement(interp,line,valuev[i]);
						}
						if (error) {return TCL_ERROR;}
					}
					Tcl_SetObjResult(interp, line);
				}
			} else {
				error = Tcl_ListObjIndex(interp,dbdata->resultlines[ituple],ifield,&element);
				if (error) {return TCL_ERROR;}
				if (element == dbdata->defnullvalue) {
					Tcl_SetObjResult(interp, nullvalue);
				} else {
					Tcl_SetObjResult(interp, element);
				}
			}
			break;
		case Isnull:
			error = Tcl_ListObjIndex(interp,dbdata->resultlines[ituple],ifield,&element);
			if (error) {return TCL_ERROR;}
			if (element == dbdata->defnullvalue) {
				Tcl_SetObjResult(interp,Tcl_NewIntObj(1));
			} else {
				Tcl_SetObjResult(interp,Tcl_NewIntObj(0));
			}
			return TCL_OK;
		case Array:
			{
			Tcl_Obj *line,**valuev,**fieldv;
			int valuec,fieldc,i;
			error = Tcl_ListObjGetElements(interp,dbdata->resultlines[ituple],&valuec,&valuev);
			if (error) {return TCL_ERROR;}
			error = Tcl_ListObjGetElements(interp,dbdata->resultfields,&fieldc,&fieldv);
			if (error) {return TCL_ERROR;}
			line = Tcl_NewObj();
			for(i = 0 ; i < fieldc ; i++) {
				error = Tcl_ListObjAppendElement(interp,line,fieldv[i]);
				if (error) {return TCL_ERROR;}
				if (valuev[i] == dbdata->defnullvalue) {
					error = Tcl_ListObjAppendElement(interp,line,nullvalue);
				} else {
					error = Tcl_ListObjAppendElement(interp,line,valuev[i]);
				}
				if (error) {return TCL_ERROR;}
			}
			Tcl_SetObjResult(interp, line);
			}
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
}

int dbi_Sqlite_Transaction_Commit(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
	int noretain)
{
	int error;
	error = sqlite_exec(dbdata->db,"commit",NULL,NULL,&(dbdata->errormsg));
	if (error) {return TCL_ERROR;}
	return TCL_OK;
}

int dbi_Sqlite_Transaction_Rollback(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata)
{
	int error;
	error = sqlite_exec(dbdata->db,"rollback",NULL,NULL,&(dbdata->errormsg));
	if (error) {return TCL_ERROR;}
	return TCL_OK;
}

int dbi_Sqlite_Transaction_Start(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata)
{
	int error;
	error = sqlite_exec(dbdata->db,"begin",NULL,NULL,&(dbdata->errormsg));
	if (error) {return TCL_ERROR;}
	return TCL_OK;
}

int dbi_Sqlite_ParseStatement(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
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

int dbi_Sqlite_Fillresult(
	void *clientdata,
	int numcols,
	char **values,
	char **namecols)
{
	dbi_Sqlite_Data *dbdata = (dbi_Sqlite_Data *)clientdata;
	Tcl_Interp *interp = dbdata->interp;
	Tcl_Obj *line;
	int i,error,len;
	if (dbdata->resultfields == NULL) {
		line = Tcl_NewObj();
		for (i = 0; i < numcols ; i++) {
			len = strlen(namecols[i]);
			if ((namecols[i][0] == '\"') && (namecols[i][len-1] == '\"')) {
				error = Tcl_ListObjAppendElement(interp,line,Tcl_NewStringObj(namecols[i]+1,len-2));
			} else {
				error = Tcl_ListObjAppendElement(interp,line,Tcl_NewStringObj(namecols[i],len));
			}
			if (error) {Tcl_DecrRefCount(line);return 1;}
		}
		dbdata->resultfields = line;
		dbdata->nfields = numcols;
		Tcl_IncrRefCount(dbdata->resultfields);
	}
	if (dbdata->resultflat) {
		line = dbdata->result;
	} else {
		line = Tcl_NewObj();
	}
	for (i = 0; i < numcols ; i++) {
		if (values[i] != NULL) {
			error = Tcl_ListObjAppendElement(interp,line,Tcl_NewStringObj(values[i],-1));
		} else {
			error = Tcl_ListObjAppendElement(interp,line,dbdata->nullvalue);
		}
		if (error) {Tcl_DecrRefCount(line);return 1;}
	}
	if (!(dbdata->resultflat)) {
		error = Tcl_ListObjAppendElement(interp,dbdata->result,line);
		if (error) {return 1;}
	}
	return 0;
}

int dbi_Sqlite_ClearResult(
	dbi_Sqlite_Data *dbdata
)
{
	if (dbdata->result != NULL) {
		Tcl_DecrRefCount(dbdata->result);
		dbdata->result = NULL;
	}
	if (dbdata->resultfields != NULL) {
		Tcl_DecrRefCount(dbdata->resultfields);
		dbdata->resultfields = NULL;
	}
	return TCL_OK;
}

int dbi_Sqlite_Exec(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
	Tcl_Obj *cmd,
	int flags,
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	char *cmdstring = NULL, *nullstring;
	int *parsedstatement;
	int error,cmdlen,nulllen;
	int usefetch;
	int numargs;
	usefetch = flags & EXEC_USEFETCH;
	if (usefetch) {
		dbdata->resultflat = 0;
	} else {
		dbdata->resultflat = flags & EXEC_FLAT;
	}
	if (dbdata->db == NULL) {
		Tcl_AppendResult(interp,"dbi object has no open database, open a connection first", NULL);
		return TCL_ERROR;
	}
	cmdstring = Tcl_GetStringFromObj(cmd,&cmdlen);
	error = dbi_Sqlite_ParseStatement(interp,dbdata,cmdstring,cmdlen,&parsedstatement,&numargs);
	if (objc != numargs) {
		Tcl_Free((char *)parsedstatement);
		Tcl_AppendResult(interp,"wrong number of arguments given to exec", NULL);
		Tcl_AppendResult(interp," while executing command: \"",	cmdstring, "\"", NULL);
		return TCL_ERROR;
	}
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
	dbi_Sqlite_ClearResult(dbdata);
	dbdata->result = Tcl_NewObj();
	Tcl_IncrRefCount(dbdata->result);
	if (numargs > 0) {
		char *sqlstring,*cursqlstring,*argstring;
		int i,size,prvsrcpos,arglen;
		/* calculate size of buffer needed */
		size = cmdlen;
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
			} else {
				*cursqlstring++ = '\'';
				strncpy(cursqlstring,argstring,arglen);
				cursqlstring += arglen;
				*cursqlstring++ = '\'';
			}
		}
		Tcl_ResetResult(interp);
		strncpy(cursqlstring,cmdstring+prvsrcpos,cmdlen-prvsrcpos);
		cursqlstring[cmdlen-prvsrcpos] = '\0';
		Tcl_Free((char *)parsedstatement);
/*printf("sql = %s\n",sqlstring);*/
		error = sqlite_exec(dbdata->db,sqlstring,dbi_Sqlite_Fillresult,dbdata,&(dbdata->errormsg));
		Tcl_Free(sqlstring);
	} else {
		error = sqlite_exec(dbdata->db,cmdstring,dbi_Sqlite_Fillresult,dbdata,&(dbdata->errormsg));
	}
	switch (error) {
		case SQLITE_OK:
			break;
		case SQLITE_CONSTRAINT:
			Tcl_AppendResult(interp,"database error executing command \"",
				cmdstring, "\":\n",	"violation of PRIMARY or UNIQUE KEY constraint", NULL);
			free(dbdata->errormsg);
			goto error;
			break;
		default:
			Tcl_AppendResult(interp,"database error executing command \"",
				cmdstring, "\":\n",	dbdata->errormsg, NULL);
			free(dbdata->errormsg);
			goto error;
	}
	if (!usefetch) {
		if (dbdata->result != NULL) {
			Tcl_SetObjResult(interp,dbdata->result);
			Tcl_DecrRefCount(dbdata->result);
			dbdata->result = NULL;
		}
	} else {
		dbdata->tuple = -1;
	}
	return TCL_OK;
	error:
		dbi_Sqlite_ClearResult(dbdata);
		return TCL_ERROR;
}


int dbi_Sqlite_Supports(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
	Tcl_Obj *keyword)
{
	static char *keywords[] = {
		"lines","sharedtransactions","backfetch","sharedserials",
		(char *) NULL};
	enum keywordsIdx {
		Lines,Sharedtransactions,Backfetch,Sharedserials
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

int dbi_Sqlite_Info(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
	int objc,
	Tcl_Obj **objv)
{
	Tcl_Obj *cmd,*dbcmd;
	int error,i;
	dbcmd = Tcl_NewObj();
	Tcl_GetCommandFullName(interp, dbdata->token, dbcmd);
	cmd = Tcl_NewStringObj("::dbi::sqlite::info",-1);
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

int dbi_Sqlite_Tables(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata)
{
	int error;
	Tcl_Obj *cmd = Tcl_NewStringObj("SELECT name FROM sqlite_master WHERE type in ('table','view') and name not like '_dbi_%' ORDER BY name",-1);
	error = dbi_Sqlite_Exec(interp,dbdata,cmd,EXEC_FLAT,NULL,0,NULL);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Sqlite_Cmd(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
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

int dbi_Sqlite_Serial(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
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
			error = dbi_Sqlite_TclEval(interp,dbdata,"::dbi::sqlite::serial_add",objc,objv);	
			break;
		case Delete:
			error = dbi_Sqlite_TclEval(interp,dbdata,"::dbi::sqlite::serial_delete",objc,objv);	
			break;
		case Set:
			error = dbi_Sqlite_TclEval(interp,dbdata,"::dbi::sqlite::serial_set",objc,objv);	
			break;
		case Next:
			error = dbi_Sqlite_TclEval(interp,dbdata,"::dbi::sqlite::serial_next",objc,objv);	
			break;
		case Share:
			error = dbi_Sqlite_TclEval(interp,dbdata,"::dbi::sqlite::serial_share",objc,objv);	
			break;
	}
	return error;
}

int dbi_Sqlite_Createdb(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	static char *switches[] = {"-sync", (char *) NULL};
    enum switchesIdx {Sync};
	char *errormsg,*syncstring = "PRAGMA default_synchronous = OFF";
	int i,index;
	if (dbdata->db != NULL) {
		Tcl_AppendResult(interp,"dbi object has open connection, close first", NULL);
		goto error;
	}
	if (objc < 3) {
		Tcl_WrongNumArgs(interp,2,objv,"database ?options?");
		return TCL_ERROR;
	}
	dbdata->database = objv[2];
	Tcl_IncrRefCount(dbdata->database);
	dbdata->autocommit = 1;
	dbdata->db = sqlite_open(Tcl_GetStringFromObj(dbdata->database,NULL), 1, &errormsg);
	if ((dbdata->db == NULL) || (errormsg != NULL)) {
		Tcl_AppendResult(interp,"connection to database \"",
			Tcl_GetStringFromObj(dbdata->database,NULL) , "\" failed:\n", errormsg, NULL);
		free(errormsg);
		Tcl_DecrRefCount(dbdata->database);dbdata->database=NULL;
		goto error;
	}
	for (i = 3; i < objc; i++) {
		if (Tcl_GetIndexFromObj(interp, objv[i], switches, "option", 0, &index)!= TCL_OK) {
			sqlite_close(dbdata->db);
			dbdata->db = NULL;
			return TCL_ERROR;
		}
		switch (index) {
			case Sync:
				syncstring = "PRAGMA default_synchronous = ON";
				break;
			default:
				break;
		}
	}
	sqlite_exec(dbdata->db,syncstring,NULL,dbdata,&(dbdata->errormsg));
	sqlite_close(dbdata->db);
	dbdata->db = NULL;
	return TCL_OK;
	error:
		return TCL_ERROR;
}

int dbi_Sqlite_Close(
	dbi_Sqlite_Data *dbdata)
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
		if (dbdata->db != NULL) {
			sqlite_close(dbdata->db);
			dbdata->db = NULL;
		}
		dbi_Sqlite_ClearResult(dbdata);
	} else {
		/* this is a clone, so remove the clone from its parent */
		dbi_Sqlite_Data *parent = dbdata->parent;
		int i;
		for (i = 0 ; i < parent->clonesnum; i++) {
			if (parent->clones[i] == dbdata) break;
		}
		i++;
		for (; i < parent->clonesnum; i++) {
			parent->clones[i-1] = parent->clones[i];
		}
		parent->clonesnum--;
		dbi_Sqlite_ClearResult(dbdata);
	}
	return TCL_OK;
}

int dbi_Sqlite_Dropdb(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata)
{
	Tcl_Obj *database;
	int error;
	if (dbdata->parent != NULL) {
		Tcl_AppendResult(interp,"cannot drop database from a clone", NULL);
		return TCL_ERROR;
	}
	if (dbdata->db == NULL) {
		Tcl_AppendResult(interp,"dbi object has no open connection", NULL);
		return TCL_ERROR;
	}
	database = dbdata->database;
	Tcl_IncrRefCount(database);
	error = dbi_Sqlite_Close(dbdata);
	if (error) {Tcl_DecrRefCount(database);return error;}
	error = Tcl_VarEval(interp,"file delete ",Tcl_GetStringFromObj(database,NULL),NULL);
	Tcl_DecrRefCount(database);
	if (error) {return error;}
	return TCL_OK;
}

int dbi_Sqlite_Destroy(
	ClientData clientdata)
{
	dbi_Sqlite_Data *dbdata = (dbi_Sqlite_Data *)clientdata;
	if (dbdata->database != NULL) {
		dbi_Sqlite_Close(dbdata);
	}
	if (dbdata->parent == NULL) {
		/* this is not a clone, so really remove defnullvalue */
		Tcl_DecrRefCount(dbdata->defnullvalue);
	}
	Tcl_Free((char *)dbdata);
	Tcl_DeleteExitHandler((Tcl_ExitProc *)dbi_Sqlite_Destroy, clientdata);
	return TCL_OK;
}

int dbi_Sqlite_Interface(
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

int Dbi_sqlite_DbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_Sqlite_Data *dbdata = (dbi_Sqlite_Data *)clientdata;
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
		return dbi_Sqlite_Interface(interp,objc,objv);
    case Open:
		if (dbdata->parent != NULL) {
			Tcl_AppendResult(interp,"clone may not use open",NULL);
			return TCL_ERROR;
		}
		return dbi_Sqlite_Open(interp,dbdata,objc,objv);
	case Create:
		return dbi_Sqlite_Createdb(interp,dbdata,objc,objv);
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
				if (strncmp(Tcl_GetStringFromObj(dbcmd,NULL),"::dbi::sqlite::priv_",23) == 0) continue;
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
		return dbi_Sqlite_Exec(interp,dbdata,objv[i],flags,nullvalue,objc-i-1,objv+i+1);
		}
	case Fetch:
		return dbi_Sqlite_Fetch(interp,dbdata, objc, objv);
	case Info:
		return dbi_Sqlite_Info(interp,dbdata,objc-2,objv+2);
	case Tables:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_Sqlite_Tables(interp,dbdata);
	case Fields:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "tablename");
			return TCL_ERROR;
		}
		return dbi_Sqlite_Cmd(interp,dbdata,objv[2],NULL,"::dbi::sqlite::fieldsinfo");
	case Close:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->parent == NULL) {
			return dbi_Sqlite_Close(dbdata);
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
		error = dbi_Sqlite_Transaction_Commit(interp,dbdata,1);
		if (error) {dbi_Sqlite_Error(interp,dbdata,"committing transaction");return error;}
		error = dbi_Sqlite_Transaction_Start(interp,dbdata);
		if (error) {dbi_Sqlite_Error(interp,dbdata,"starting transaction");return error;}
		dbi_Sqlite_autocommit(dbdata,0);
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
		dbi_Sqlite_autocommit(dbdata,1);
		error = dbi_Sqlite_Transaction_Commit(interp,dbdata,1);
		if (error) {dbi_Sqlite_Error(interp,dbdata,"committing transaction");return error;}
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
		dbi_Sqlite_autocommit(dbdata,1);
		error = dbi_Sqlite_Transaction_Rollback(interp,dbdata);
		if (error) {dbi_Sqlite_Error(interp,dbdata,"rolling back transaction");return error;}
		return TCL_OK;
	case Serial:
		if (objc < 5) {
			Tcl_WrongNumArgs(interp, 2, objv, "option table field ?value?");
			return TCL_ERROR;
		}
		return dbi_Sqlite_Serial(interp,dbdata,objv[2],objc-3,objv+3);
	case Drop:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_Sqlite_Dropdb(interp,dbdata);
	case Clone:
		if ((objc != 2) && (objc != 3)) {
			Tcl_WrongNumArgs(interp, 2, objv, "?name?");
			return TCL_ERROR;
		}
		if (objc == 3) {
			return Dbi_sqlite_Clone(interp,dbdata,objv[2]);
		} else {
			return Dbi_sqlite_Clone(interp,dbdata,NULL);
		}
	case Supports:
		if (objc == 2) {
			return dbi_Sqlite_Supports(interp,dbdata,NULL);
		} if (objc == 3) {
			return dbi_Sqlite_Supports(interp,dbdata,objv[2]);
		} else {
			Tcl_WrongNumArgs(interp, 2, objv, "?keyword?");
			return TCL_ERROR;
		}
	}
	return error;
}

static int dbi_num = 0;
int Dbi_sqlite_DoNewDbObjCmd(
	dbi_Sqlite_Data *dbdata,
	Tcl_Interp *interp,
	Tcl_Obj *dbi_nameObj)
{
	char buffer[40];
	char *dbi_name;
	dbdata->interp = interp;
	dbdata->db = NULL;
	dbdata->autocommit = 1;
	dbdata->database=NULL;
	dbdata->defnullvalue = NULL;
	dbdata->result = NULL;
	dbdata->resultfields = NULL;
	dbdata->resultflat = 0;
	dbdata->nullvalue = NULL;
	dbdata->clones = NULL;
	dbdata->clonesnum = 0;
	dbdata->parent = NULL;
	if (dbi_nameObj == NULL) {
		dbi_num++;
		sprintf(buffer,"::dbi::sqlite::dbi%d",dbi_num);
		dbi_nameObj = Tcl_NewStringObj(buffer,strlen(buffer));
	}
	dbi_name = Tcl_GetStringFromObj(dbi_nameObj,NULL);
	dbdata->token = Tcl_CreateObjCommand(interp,dbi_name,(Tcl_ObjCmdProc *)Dbi_sqlite_DbObjCmd,
		(ClientData)dbdata,(Tcl_CmdDeleteProc *)dbi_Sqlite_Destroy);
	Tcl_CreateExitHandler((Tcl_ExitProc *)dbi_Sqlite_Destroy, (ClientData)dbdata);
	Tcl_SetObjResult(interp,dbi_nameObj);
	return TCL_OK;
}

int Dbi_sqlite_NewDbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_Sqlite_Data *dbdata;
	int error;
	if ((objc < 1)||(objc > 2)) {
		Tcl_WrongNumArgs(interp,2,objv,"?dbName?");
		return TCL_ERROR;
	}
	dbdata = (dbi_Sqlite_Data *)Tcl_Alloc(sizeof(dbi_Sqlite_Data));
	if (objc == 2) {
		error = Dbi_sqlite_DoNewDbObjCmd(dbdata,interp,objv[1]);
	} else {
		error = Dbi_sqlite_DoNewDbObjCmd(dbdata,interp,NULL);
	}
	dbdata->defnullvalue = Tcl_NewObj();
	Tcl_IncrRefCount(dbdata->defnullvalue);
	if (error) {
		Tcl_Free((char *)dbdata);
	}
	return error;
}

int Dbi_sqlite_Clone(
	Tcl_Interp *interp,
	dbi_Sqlite_Data *dbdata,
	Tcl_Obj *name)
{
	dbi_Sqlite_Data *parent = NULL;
	dbi_Sqlite_Data *clone_dbdata = NULL;
	int error;
	parent = dbdata;
	while (parent->parent != NULL) {parent = parent->parent;}
	clone_dbdata = (dbi_Sqlite_Data *)Tcl_Alloc(sizeof(dbi_Sqlite_Data));
	error = Dbi_sqlite_DoNewDbObjCmd(clone_dbdata,interp,name);
	if (error) {Tcl_Free((char *)clone_dbdata);return TCL_ERROR;}
	parent->clonesnum++;
	parent->clones = (dbi_Sqlite_Data **)Tcl_Realloc((char *)parent->clones,parent->clonesnum*sizeof(dbi_Sqlite_Data **));
	parent->clones[parent->clonesnum-1] = clone_dbdata;
	clone_dbdata->parent = parent;
	clone_dbdata->db = parent->db;
	clone_dbdata->database = parent->database;
	clone_dbdata->result = NULL;
	clone_dbdata->resultfields = NULL;
	dbdata->resultflat = 0;
	clone_dbdata->nullvalue = NULL;
	clone_dbdata->defnullvalue = parent->defnullvalue;
	return TCL_OK;
	error:
		Tcl_DeleteCommandFromToken(interp,clone_dbdata->token);
		Tcl_Free((char *)clone_dbdata);
		return TCL_ERROR;
}

int Dbi_sqlite_Init(interp)
	Tcl_Interp *interp;		/* Interpreter to add extra commands */
{
#ifdef USE_TCL_STUBS
	if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
		return TCL_ERROR;
	}
#endif
	Tcl_CreateObjCommand(interp,"dbi_sqlite",(Tcl_ObjCmdProc *)Dbi_sqlite_NewDbObjCmd,
		(ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
	Tcl_Eval(interp,"");
	return TCL_OK;
}
