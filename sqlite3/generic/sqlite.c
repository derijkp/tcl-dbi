/*
 *       File:    sqlite.c
 *       Purpose: dbi extension to Tcl: sqlite3 backend
 *       Author:  Copyright (c) 1998 Peter De Rijk
 *
 *       See the file "README" for information on usage and redistribution
 *       of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "dbi_sqlite.h"

int dbi_Sqlite3_getrow(
	Tcl_Interp *interp,
	sqlite3_stmt *stmt,
	Tcl_Obj *nullvalue,
	Tcl_Obj **result);

typedef struct vary {
	short vary_length;
	char vary_string[1];
} VARY;

int Dbi_sqlite3_Clone(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	Tcl_Obj *name);

int dbi_Sqlite3_Free_Stmt(
	dbi_Sqlite3_Data *dbdata,
	int action);

/* following function slightly adapted from tclsqlite */
static SqlFunc *findSqlFunc(dbi_Sqlite3_Data *pDb, const char *zName){
  SqlFunc *p, *pNew;
  int i;
  pNew = (SqlFunc*)Tcl_Alloc( sizeof(*pNew) + strlen(zName) + 1 );
  pNew->zName = (char*)&pNew[1];
  for(i=0; zName[i]; i++){ pNew->zName[i] = tolower(zName[i]); }
  pNew->zName[i] = 0;
  for(p=pDb->pFunc; p; p=p->pNext){ 
    if( strcmp(p->zName, pNew->zName)==0 ){
      Tcl_Free((char*)pNew);
      return p;
    }
  }
  pNew->interp = pDb->interp;
  pNew->pScript = 0;
  pNew->pNext = pDb->pFunc;
  pDb->pFunc = pNew;
  return pNew;
}

/* following function slightly adapted from tclsqlite */
static int safeToUseEvalObjv(Tcl_Interp *interp, Tcl_Obj *pCmd){
  /* We could try to do something with Tcl_Parse().  But we will instead
  ** just do a search for forbidden characters.  If any of the forbidden
  ** characters appear in pCmd, we will report the string as unsafe.
  */
  const char *z;
  int n;
  z = Tcl_GetStringFromObj(pCmd, &n);
  while( n-- > 0 ){
    int c = *(z++);
    if( c=='$' || c=='[' || c==';' ) return 0;
  }
  return 1;
}

/******************************************************************/

int Dbi_sqlite3_collate_dictionary(void *userdata,int llen,const void *s1,int rlen,const void *s2)
{
    char *left = (char *)s1;
    char *right = (char *)s2;
    int diff, zeros,l = 0,r = 0;
    int secondaryDiff = 0;
    while (1) {
	if (isdigit(UCHAR(right[r])) && isdigit(UCHAR(left[l]))) {
	    /*
	     * There are decimal numbers embedded in the two
	     * strings.  Compare them as numbers, rather than
	     * strings.  If one number has more leading zeros than
	     * the other, the number with more leading zeros sorts
	     * later, but only as a secondary choice.
	     */
	    zeros = 0;
	    while ((right[r] == '0') && (r < rlen)) {
		r++;
		zeros--;
	    }
	    while ((left[l] == '0') && (l < llen)) {
		l++;
		zeros++;
	    }
	    if (secondaryDiff == 0) {
		secondaryDiff = zeros;
	    }
	    /*
	     * The code below compares the numbers in the two
	     * strings without ever converting them to integers.  It
	     * does this by first comparing the lengths of the
	     * numbers and then comparing the digit values.
	     */
	    diff = 0;
	    while (1) {
		if (diff == 0) {
		    diff = left[l] - right[r];
		}
		r++;
		l++;
		if (!isdigit(UCHAR(right[r]))) {
		    if (isdigit(UCHAR(left[l]))) {
			return 1;
		    } else {
			/*
			 * The two numbers have the same length. See
			 * if their values are different.
			 */
			if (diff != 0) {
			    return diff;
			}
			break;
		    }
		} else if (!isdigit(UCHAR(left[l]))) {
		    return -1;
		}
	    }
	    continue;
	}
        diff = left[l] - right[r];
        if (diff) {
            if (isupper(UCHAR(left[l])) && islower(UCHAR(right[r]))) {
                diff = tolower(left[l]) - right[r];
                if (diff) {
		    return diff;
                } else if (secondaryDiff == 0) {
		    secondaryDiff = -1;
                }
            } else if (isupper(UCHAR(right[r])) && islower(UCHAR(left[l]))) {
                diff = left[l] - tolower(UCHAR(right[r]));
                if (diff) {
		    return diff;
                } else if (secondaryDiff == 0) {
		    secondaryDiff = 1;
                }
            } else {
                return diff;
            }
        }
        if (left[l] == 0) {
	    break;
	}
        l++;
        r++;
    }
    if (diff == 0) {
	diff = secondaryDiff;
    }
    return diff;	
}

void Dbi_sqlite3_regexp(sqlite3_context *context,int argc,sqlite3_value **argv)
{
	dbi_Sqlite3_Data *dbdata = sqlite3_user_data(context);
	int result;
	if (argc != 2) {
		sqlite3_result_error(context, "wrong number of arguments to function regexp, must be: pattern string", -1);
		return;
	}
	result = Tcl_RegExpMatch(dbdata->interp, (char *)sqlite3_value_text(argv[1]), (char *)sqlite3_value_text(argv[0]));
	if (result == -1) {
		sqlite3_result_error(context, Tcl_GetStringResult(dbdata->interp), -1);
	} else if (result == 1) {
		sqlite3_result_int(context, 1);
	} else {
		sqlite3_result_int(context, 0);
	}
	return;
}

/* following function slightly adapted from tclsqlite */
static void Dbi_sqlite3_tclSqlFunc(sqlite3_context *context, int argc, sqlite3_value**argv){
	SqlFunc *p = sqlite3_user_data(context);
	Tcl_Obj *pCmd;
	int i;
	int rc;
	
	if( argc==0 ){
		/* If there are no arguments to the function, call Tcl_EvalObjEx on the
		** script object directly. This allows the TCL compiler to generate
		** bytecode for the command on the first invocation and thus make
		** subsequent invocations much faster. */
		pCmd = p->pScript;
		Tcl_IncrRefCount(pCmd);
		rc = Tcl_EvalObjEx(p->interp, pCmd, 0);
		Tcl_DecrRefCount(pCmd);
	}else{
		/* If there are arguments to the function, make a shallow copy of the
		** script object, lappend the arguments, then evaluate the copy.
		**
		** By "shallow" copy, we mean a only the outer list Tcl_Obj is duplicated.
		** The new Tcl_Obj contains pointers to the original list elements. 
		** That way, when Tcl_EvalObjv() is run and shimmers the first element
		** of the list to tclCmdNameType, that alternate representation will
		** be preserved and reused on the next invocation.
		*/
		Tcl_Obj **aArg;
		int nArg;
		if( Tcl_ListObjGetElements(p->interp, p->pScript, &nArg, &aArg) ){
			sqlite3_result_error(context, Tcl_GetStringResult(p->interp), -1); 
			return;
		}
		pCmd = Tcl_NewListObj(nArg, aArg);
		Tcl_IncrRefCount(pCmd);
		for(i=0; i<argc; i++){
			sqlite3_value *pIn = argv[i];
			Tcl_Obj *pVal;
		 	 		
			/* Set pVal to contain the i'th column of this row. */
			switch( sqlite3_value_type(pIn) ){
		 	 case SQLITE_BLOB: {
		 	 	int bytes = sqlite3_value_bytes(pIn);
		 	 	pVal = Tcl_NewByteArrayObj(sqlite3_value_blob(pIn), bytes);
		 	 	break;
		 	 }
		 	 case SQLITE_INTEGER: {
		 	 	sqlite_int64 v = sqlite3_value_int64(pIn);
		 	 	if( v>=-2147483647 && v<=2147483647 ){
		 	 		pVal = Tcl_NewIntObj(v);
		 	 	}else{
		 	 		pVal = Tcl_NewWideIntObj(v);
		 	 	}
		 	 	break;
		 	 }
		 	 case SQLITE_FLOAT: {
		 	 	double r = sqlite3_value_double(pIn);
		 	 	pVal = Tcl_NewDoubleObj(r);
		 	 	break;
		 	 }
		 	 case SQLITE_NULL: {
		 	 	pVal = Tcl_NewStringObj("", 0);
		 	 	break;
		 	 }
		 	 default: {
		 	 	int bytes = sqlite3_value_bytes(pIn);
		 	 	pVal = Tcl_NewStringObj((char *)sqlite3_value_text(pIn), bytes);
		 	 	break;
		 	 }
			}
			rc = Tcl_ListObjAppendElement(p->interp, pCmd, pVal);
			if( rc ){
		 	 Tcl_DecrRefCount(pCmd);
		 	 sqlite3_result_error(context, Tcl_GetStringResult(p->interp), -1); 
		 	 return;
			}
		}
		if( !p->useEvalObjv ){
			/* Tcl_EvalObjEx() will automatically call Tcl_EvalObjv() if pCmd
			** is a list without a string representation.	To prevent this from
			** happening, make sure pCmd has a valid string representation */
			Tcl_GetString(pCmd);
		}
		rc = Tcl_EvalObjEx(p->interp, pCmd, TCL_EVAL_DIRECT);
		Tcl_DecrRefCount(pCmd);
	}
	
	if( rc && rc!=TCL_RETURN ){
		sqlite3_result_error(context, Tcl_GetStringResult(p->interp), -1); 
	}else{
		Tcl_Obj *pVar = Tcl_GetObjResult(p->interp);
		int n;
		unsigned char *data;
		char *zType = pVar->typePtr ? pVar->typePtr->name : "";
		char c = zType[0];
		if( c=='b' && strcmp(zType,"bytearray")==0 && pVar->bytes==0 ){
			/* Only return a BLOB type if the Tcl variable is a bytearray and
			** has no string representation. */
			data = Tcl_GetByteArrayFromObj(pVar, &n);
			sqlite3_result_blob(context, data, n, SQLITE_TRANSIENT);
		}else if( (c=='b' && strcmp(zType,"boolean")==0) ||
		 	 	(c=='i' && strcmp(zType,"int")==0) ){
			Tcl_GetIntFromObj(0, pVar, &n);
			sqlite3_result_int(context, n);
		}else if( c=='d' && strcmp(zType,"double")==0 ){
			double r;
			Tcl_GetDoubleFromObj(0, pVar, &r);
			sqlite3_result_double(context, r);
		}else if( c=='w' && strcmp(zType,"wideInt")==0 ){
			Tcl_WideInt v;
			Tcl_GetWideIntFromObj(0, pVar, &v);
			sqlite3_result_int64(context, v);
		}else{
			data = (unsigned char *)Tcl_GetStringFromObj(pVar, &n);
			sqlite3_result_text(context, (char *)data, n, SQLITE_TRANSIENT);
		}
	}
}

/* following function slightly adapted from tclsqlite */
static int Dbi_sqlite3_tclSqlCollate(
  void *pCtx,
  int nA,
  const void *zA,
  int nB,
  const void *zB
){
  SqlCollate *p = (SqlCollate *)pCtx;
  Tcl_Obj *pCmd;

  pCmd = Tcl_NewStringObj(p->zScript, -1);
  Tcl_IncrRefCount(pCmd);
  Tcl_ListObjAppendElement(p->interp, pCmd, Tcl_NewStringObj(zA, nA));
  Tcl_ListObjAppendElement(p->interp, pCmd, Tcl_NewStringObj(zB, nB));
  Tcl_EvalObjEx(p->interp, pCmd, TCL_EVAL_DIRECT);
  Tcl_DecrRefCount(pCmd);
  return (atoi(Tcl_GetStringResult(p->interp)));
}

int dbi_Sqlite3_Error(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	char *premsg)
{
	if (strlen(premsg) != 0) {
		Tcl_AppendResult(interp,"error ",premsg,":\n", NULL);
	}
	if (dbdata->errormsg != NULL) {
		Tcl_AppendResult(interp,"error ",dbdata->errormsg,NULL);
		sqlite3_free(dbdata->errormsg);
		dbdata->errormsg = NULL;
	} else {
		const char *msg = sqlite3_errmsg(dbdata->db);
		Tcl_AppendResult(interp,"error ",msg,NULL);
	}
	return TCL_ERROR;
}

int dbi_Sqlite3_bindarg(
	Tcl_Interp *interp,
	sqlite3_stmt *stmt,
	int i,
	Tcl_Obj *pVar,
	char *nullstring,
	int nulllen)
{
	char *zType, c;
	int n;
	if (nullstring != NULL) {
		char *argstring;
		int arglen;
		argstring = Tcl_GetStringFromObj(pVar,&arglen);
		if ((arglen == nulllen)&&(strncmp(argstring,nullstring,arglen) == 0)) {
			zType = "N";
		} else {
			zType = pVar->typePtr ? pVar->typePtr->name : "";
		}
	} else {
		zType = pVar->typePtr ? pVar->typePtr->name : "";
	}
	c = zType[0];
	if (c=='N' && strcmp(zType,"N")==0) {
		sqlite3_bind_null(stmt, i);
	} else if (c=='b' && strcmp(zType,"bytearray")==0) {
		unsigned char *data;
		data = Tcl_GetByteArrayFromObj(pVar, &n);
		sqlite3_bind_blob(stmt, i, data, n, SQLITE_STATIC);
	} else if ((c=='b' && strcmp(zType,"boolean")==0) || (c=='i' && strcmp(zType,"int")==0)) {
		Tcl_GetIntFromObj(interp, pVar, &n);
		sqlite3_bind_int(stmt, i, n);
	} else if (c=='d' && strcmp(zType,"double")==0) {
		double r;
		Tcl_GetDoubleFromObj(interp, pVar, &r);
		sqlite3_bind_double(stmt, i, r);
	} else {
		char *data;
		data = Tcl_GetStringFromObj(pVar, &n);
		sqlite3_bind_text(stmt, i, data, n, SQLITE_STATIC);
	}
	return TCL_OK;
}

int dbi_Sqlite3_TclEval(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
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

int dbi_Sqlite3_Open(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	int error;
	if (dbdata->db != NULL) {
		Tcl_AppendResult(interp,"dbi object has open connection, close first", NULL);
		goto error;
	}
	if (objc < 3) {
		Tcl_WrongNumArgs(interp,2,objv,"database");
		return TCL_ERROR;
	}
	error = dbi_Sqlite3_TclEval(interp,dbdata,"::dbi::sqlite3::open_test",objc-2,objv+2);	
	if (error) {return error;}
	dbdata->database = Tcl_GetObjResult(interp);
	Tcl_IncrRefCount(dbdata->database);
	error = sqlite3_open(Tcl_GetStringFromObj(dbdata->database,NULL), &(dbdata->db));
	if (error != SQLITE_OK) {
		Tcl_AppendResult(interp,"connection to database \"",
			Tcl_GetStringFromObj(dbdata->database,NULL) , "\" failed:\n", sqlite3_errmsg(dbdata->db), NULL);
		sqlite3_close(dbdata->db);
		Tcl_DecrRefCount(dbdata->database);dbdata->database=NULL;
		goto error;
	}
	sqlite3_create_collation(dbdata->db,"DICT",SQLITE_UTF8,(void *)NULL,Dbi_sqlite3_collate_dictionary);
	sqlite3_create_function(dbdata->db,"regexp",2,SQLITE_UTF8,dbdata,Dbi_sqlite3_regexp,(void *)NULL,(void *)NULL);
	error = sqlite3_exec(dbdata->db,"PRAGMA empty_result_callbacks = ON",NULL,NULL,&(dbdata->errormsg));
	if (error) {sqlite3_free(dbdata->errormsg); dbdata->errormsg=NULL;}
	error = dbi_Sqlite3_TclEval(interp,dbdata,"::dbi::sqlite3::update",0,NULL);	
	return error;
	error:
		return TCL_ERROR;
}

int dbi_Sqlite3_Fetch(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	Tcl_Obj *element;
	Tcl_Obj *tuple = NULL, *field = NULL, *nullvalue = NULL;
	int fetch_option,ituple=-1,ifield=-1;
	int error;
	static CONST char *subCmds[] = {
		 "data", "lines", "pos", "fields", "isnull", "array",
		(char *) NULL};
	enum ISubCmdIdx {
		Data, Lines, Pos, Fields, Isnull, Array
	};
	static CONST char *switches[] = {
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
			if (dbdata->resultfields != NULL) {
				Tcl_SetObjResult(interp,dbdata->resultfields);
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
			if (dbdata->resultfields == NULL) {
				return TCL_OK;
			}
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

int dbi_Sqlite3_Transaction_Commit(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	int noretain)
{
	int error;
/*fprintf(stdout,":commit\n");fflush(stdout);*/
	error = sqlite3_exec(dbdata->db,"commit transaction _dbi",NULL,NULL,&(dbdata->errormsg));
	if (error) {
		if (strcmp("cannot commit - no transaction is active",dbdata->errormsg) == 0) {
			sqlite3_free(dbdata->errormsg); dbdata->errormsg=NULL;
			return TCL_OK;
		} else {
			return TCL_ERROR;
		}
	}
	return TCL_OK;
}

int dbi_Sqlite3_Transaction_Rollback(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata)
{
	int error;
/*fprintf(stdout,":rollback\n");fflush(stdout);*/
	error = sqlite3_exec(dbdata->db,"rollback transaction _dbi",NULL,NULL,&(dbdata->errormsg));
	if (error) {
		if (strcmp("cannot rollback - no transaction is active",dbdata->errormsg) == 0) {
			sqlite3_free(dbdata->errormsg); dbdata->errormsg=NULL;
			return TCL_OK;
		} else {
			return TCL_ERROR;
		}
	}
	return TCL_OK;
}

int dbi_Sqlite3_Transaction_Start(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata)
{
	int error;
/*fprintf(stdout,":begin\n");fflush(stdout);*/
	error = sqlite3_exec(dbdata->db,"begin transaction _dbi",NULL,NULL,&(dbdata->errormsg));
	if (error) {return TCL_ERROR;}
	return TCL_OK;
}

int dbi_Sqlite3_getcolnames(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata)
{
	Tcl_Obj *line;
	char *s;
	int i,error,len,numcols;
	line = Tcl_NewObj();
	numcols = sqlite3_column_count(dbdata->stmt);
	for (i = 0; i < numcols ; i++) {
		s = (char *)sqlite3_column_name(dbdata->stmt,i);
		len = strlen(s);
		if ((s[0] == '\"') && (s[len-1] == '\"')) {
			error = Tcl_ListObjAppendElement(interp,line,Tcl_NewStringObj(s+1,len-2));
		} else {
			error = Tcl_ListObjAppendElement(interp,line,Tcl_NewStringObj(s,len));
		}
		if (error) {Tcl_DecrRefCount(line);return 1;}
	}
	dbdata->resultfields = line;
	dbdata->nfields = numcols;
	Tcl_IncrRefCount(dbdata->resultfields);
	dbdata->ntuples = 0;
	return TCL_OK;
}

int dbi_Sqlite3_ClearResult(
	dbi_Sqlite3_Data *dbdata
)
{
	if (dbdata->result != NULL) {
		Tcl_DecrRefCount(dbdata->result);
		dbdata->result = NULL;
	}
	if (dbdata->stmt != NULL) {
		sqlite3_finalize(dbdata->stmt);
		dbdata->stmt = NULL;
	}
	if (dbdata->resultfields != NULL) {
		Tcl_DecrRefCount(dbdata->resultfields);
		dbdata->resultfields = NULL;
	}
	return TCL_OK;
}

int dbi_Sqlite3_SplitObject(
	Tcl_Interp *interp,
	Tcl_Obj *object,
	Tcl_Obj **table,
	Tcl_Obj **id
)
{
	Tcl_Obj **objv;
	int objc,error;
	error = Tcl_ListObjGetElements(interp,object,&objc,&objv);
	if (error) return TCL_ERROR;
	if (objc != 2) {
		Tcl_AppendResult(interp,"object must be a list of the form {table id}", NULL);
		return TCL_ERROR;
	}
	*table = objv[0];
	*id = objv[1];
	return TCL_OK;		
}

int dbi_Sqlite3_getresultfield(
	Tcl_Interp *interp,
	sqlite3_stmt *stmt,
	int i,
	Tcl_Obj **result)
{
	Tcl_Obj *pVal;
	pVal = NULL;
	switch (sqlite3_column_type(stmt, i)) {
		case SQLITE_BLOB: {
			int bytes = sqlite3_column_bytes(stmt, i);
			pVal = Tcl_NewByteArrayObj(sqlite3_column_blob(stmt, i), bytes);
			break;
		}
		case SQLITE_INTEGER: {
			sqlite_int64 v = sqlite3_column_int64(stmt, i);
			if (v >= -2147483647 && v <= 2147483647) {
				pVal = Tcl_NewIntObj(v);
			} else {
				pVal = Tcl_NewWideIntObj(v);
			}
			break;
		}
		case SQLITE_FLOAT: {
			double r = sqlite3_column_double(stmt, i);
			pVal = Tcl_NewDoubleObj(r);
			break;
		}
		case SQLITE_NULL: {
			pVal = NULL;
			break;
		}
		default: {
			int bytes = sqlite3_column_bytes(stmt, i);
			char *s = (char *)sqlite3_column_text(stmt, i);
			pVal = Tcl_NewStringObj(s,bytes);
			break;
		}
	}
	*result = pVal;
	return TCL_OK;
}

int dbi_Sqlite3_Get(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	Tcl_Obj *object,
	int objc,
	Tcl_Obj **objv)
{
	sqlite3_stmt *stmt = NULL;
	const char *nextsql;
	Tcl_Obj *table,*id,*line;
	char *buffer = NULL,*idstring,*tablestring,*tempstring,*pos;
	int i,j,error,len,idlen,tablelen,templen;
	if (objc == 1) {
		error = Tcl_ListObjGetElements(interp,objv[0],&objc,&objv);
		if (error) {return TCL_ERROR;}
	}
	error = dbi_Sqlite3_SplitObject(interp,object,&table,&id);
	if (error) {goto error;}
	tablestring = Tcl_GetStringFromObj(table,&tablelen);
	idstring = Tcl_GetStringFromObj(id,&idlen);
	len = 33 + tablelen + idlen;
	for (i = 0 ; i < objc ; i++) {
		Tcl_GetStringFromObj(objv[i],&templen);	
		len += (templen+3);
	}
	buffer = Tcl_Alloc(len*sizeof(char));
	if (objc == 0) {
		sprintf(buffer,"select * from \"%s\" where \"id\" = '%s'",tablestring,idstring);
	} else {
		strcpy(buffer,"select ");
		pos = buffer+7;
		tempstring = Tcl_GetStringFromObj(objv[0],&templen);
		*pos++ = '\"';
		for(j = 0 ; j < templen ; j++) {
			*pos++ = tempstring[j];
		}
		*pos++ = '\"';
		for (i = 1 ; i < objc ; i++) {
			tempstring = Tcl_GetStringFromObj(objv[i],&templen);	
			*pos++ = ',';
			*pos++ = '\"';
			for(j = 0 ; j < templen ; j++) {
				*pos++ = tempstring[j];
			}
			*pos++ = '\"';
		}
		sprintf(pos," from \"%s\" where \"id\" = '%s'",tablestring,idstring);
	}
	error = sqlite3_prepare_v2(dbdata->db,buffer,-1,&stmt,&nextsql);
	if (error != SQLITE_OK) {
		dbi_Sqlite3_Error(interp,dbdata,"preparing set statement");
		goto error;
	}
	error = sqlite3_step(stmt);
	Tcl_Free(buffer); buffer = NULL;
	switch (error) {
		case SQLITE_DONE:
			Tcl_AppendResult(interp,"object {",Tcl_GetStringFromObj(object,NULL), "} not found", NULL);
			goto error;
		case SQLITE_ROW:
			break;
		default:
			Tcl_AppendResult(interp,"database error executing command \"",
				buffer, "\":\n", sqlite3_errmsg(dbdata->db), NULL);
			goto error;
	}
	if (objc == 0) {
		Tcl_Obj *el;
		char *s;
		int numcols;
		line = Tcl_NewObj();
		numcols = sqlite3_column_count(stmt);
		for (i = 0; i < numcols ; i++) {
			error = dbi_Sqlite3_getresultfield(interp,stmt,i,&el);
			if (error) {Tcl_DecrRefCount(line);goto error;}
			if (el != NULL) {
				s = (char *)sqlite3_column_name(stmt,i);
				len = strlen(s);
				if ((s[0] == '\"') && (s[len-1] == '\"')) {
					error = Tcl_ListObjAppendElement(interp,line,Tcl_NewStringObj(s+1,len-2));
				} else {
					error = Tcl_ListObjAppendElement(interp,line,Tcl_NewStringObj(s,len));
				}
				if (error) {Tcl_DecrRefCount(line);goto error;}
				error = Tcl_ListObjAppendElement(interp,line,el);
			}
			if (error) {Tcl_DecrRefCount(line);Tcl_DecrRefCount(el);goto error;}
		}
		Tcl_SetObjResult(interp, line);
	} else if (objc == 1) {
		Tcl_Obj *result, *subresult;
		error = dbi_Sqlite3_getrow(interp,stmt,dbdata->nullvalue,&result);
		if (error) {goto error;}
		error = Tcl_ListObjIndex(interp,result,0,&subresult);
		if (error) {Tcl_DecrRefCount(result);goto error;}
		Tcl_SetObjResult(interp, subresult);
		Tcl_DecrRefCount(result);
	} else {
		Tcl_Obj *result;
		error = dbi_Sqlite3_getrow(interp,stmt,dbdata->nullvalue,&result);
		if (error) {goto error;}
		Tcl_SetObjResult(interp, result);
	}
	sqlite3_finalize(stmt); stmt = NULL;
	dbi_Sqlite3_ClearResult(dbdata);
	return TCL_OK;
	error:
		if (buffer != NULL) {Tcl_Free(buffer);}
		if (stmt != NULL) {sqlite3_finalize(stmt);}
		dbi_Sqlite3_ClearResult(dbdata);
		return TCL_ERROR;
}

int dbi_Sqlite3_Insert(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	Tcl_Obj *table,
	Tcl_Obj *id,
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	sqlite3_stmt *stmt = NULL;
	const char *nextsql;
	Tcl_Obj *fid = NULL,*result;
	char *buffer = NULL,*tablestring,*tempstring,*pos,*nullstring = NULL;
	int i,j,error,len,tablelen,templen,nulllen;
	if (objc == 1) {
		error = Tcl_ListObjGetElements(interp,objv[0],&objc,&objv);
		if (error) {return TCL_ERROR;}
	}
	if ((objc == 0)||(objc % 2)) {
		Tcl_AppendResult(interp,"data for set must be: field value ?field value ...?", NULL);
		goto error;
	}
	if (nullvalue != NULL) {
		nullstring = Tcl_GetStringFromObj(nullvalue,&nulllen);
	}
	/* calculate length */
	tablestring = Tcl_GetStringFromObj(table,&tablelen);
	len = 31 + tablelen;
	for (i = 0 ; i < objc ; i+=2) {
		tempstring = Tcl_GetStringFromObj(objv[i],&templen);	
		if (templen < 2) {templen = 2;}
		len += (templen+5);
	}
	if (id != NULL) {
		tempstring = Tcl_GetStringFromObj(id,&templen);
		len += (5 + 3 + templen);
	}
	/* make sql */
	buffer = Tcl_Alloc(len*sizeof(char));
	tempstring = Tcl_GetStringFromObj(objv[0],&templen);
	if ((templen == 2) && (strncmp(tempstring,"id",2) == 0)) {fid = objv[1];}
	sprintf(buffer,"insert into \"%s\" (\"%s\"",tablestring,tempstring);
	pos = buffer + 18 + tablelen + templen;
	for (i = 2 ; i < objc ; i += 2) {
		tempstring = Tcl_GetStringFromObj(objv[i],&templen);
		if ((templen == 2) && (strncmp(tempstring,"id",2) == 0)) {fid = objv[1];}
		sprintf(pos,",\"%s\"",tempstring);
		pos += (3 + templen);
	}
	if (id != NULL) {
		sprintf(pos,",\"id\"");
		pos += 5;
	}
	tempstring = Tcl_GetStringFromObj(objv[1],&templen);
	sprintf(pos,") values (?");
	pos += 11;
	for (i = 3 ; i < objc ; i += 2) {
		sprintf(pos,",?");
		pos += 2;
	}
	if (id != NULL) {
		sprintf(pos,",?");
		pos += 2;
	}
	*pos++ = ')';
	*pos++ = '\0';
	/* run sql */
	error = sqlite3_prepare_v2(dbdata->db,buffer,-1,&stmt,&nextsql);
	if (error != SQLITE_OK) {
		dbi_Sqlite3_Error(interp,dbdata,"preparing set statement");
		goto error;
	}
	j = 1;
	for (i = 1 ; i < objc ; i += 2) {
		dbi_Sqlite3_bindarg(interp,stmt,j,objv[i],nullstring,nulllen);
		j++;
	}
	if (id != NULL) {
		dbi_Sqlite3_bindarg(interp,stmt,j,id,nullstring,nulllen);
	}
	error = sqlite3_step(stmt);
	if (error != SQLITE_DONE) {
		dbi_Sqlite3_Error(interp,dbdata,"processing insert statement");
		goto error;
	}
	error = sqlite3_finalize(stmt); stmt = NULL;
	Tcl_Free(buffer); buffer = NULL;
	switch (error) {
		case SQLITE_OK:
			break;
		default:
			Tcl_AppendResult(interp,"error inserting object in table \"",	Tcl_GetStringFromObj(table,NULL), "\":\n",sqlite3_errmsg(dbdata->db), NULL);
			goto error;
	}
	if (id != NULL) {fid = id;}
	result = Tcl_DuplicateObj(table);
	if (fid == NULL) {
		id = Tcl_NewIntObj(sqlite3_last_insert_rowid(dbdata->db));
		error = Tcl_ListObjAppendElement(interp,result,id);
		if (error) {Tcl_DecrRefCount(result); Tcl_DecrRefCount(id); return error;}
	} else {
		error = Tcl_ListObjAppendElement(interp,result,fid);
		if (error) {Tcl_DecrRefCount(result); return error;}
	}
	Tcl_SetObjResult(interp,result);
	dbi_Sqlite3_ClearResult(dbdata);
	return TCL_OK;
	error:
		if (buffer != NULL) {Tcl_Free(buffer);}
		if (stmt != NULL) {sqlite3_finalize(stmt);}
		dbi_Sqlite3_ClearResult(dbdata);
		return TCL_ERROR;
}

int dbi_Sqlite3_Set(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	Tcl_Obj *object,
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	sqlite3_stmt *stmt = NULL;
	const char *nextsql;
	Tcl_Obj *table,*id;
	char *buffer = NULL,*idstring,*tablestring,*fieldstring,*pos,*nullstring = NULL;
	int i,j,error,len,idlen,tablelen,fieldlen,changes,nulllen;
	if (objc == 1) {
		error = Tcl_ListObjGetElements(interp,objv[0],&objc,&objv);
		if (error) {return TCL_ERROR;}
	}
	if (objc == 0) {return TCL_OK;}
	if (objc % 2) {
		Tcl_AppendResult(interp,"data for set must be: field value ?field value ...?", NULL);
		goto error;
	}
	error = dbi_Sqlite3_SplitObject(interp,object,&table,&id);
	if (error) {goto error;}
	if (nullvalue != NULL) {
		nullstring = Tcl_GetStringFromObj(nullvalue,&nulllen);
	}
	/* calculate length */
	tablestring = Tcl_GetStringFromObj(table,&tablelen);
	idstring = Tcl_GetStringFromObj(id,&idlen);
	len = 36 + tablelen + idlen;
	for (i = 0 ; i < objc ; i+=2) {
		fieldstring = Tcl_GetStringFromObj(objv[i],&fieldlen);
		if (fieldlen < 2) {fieldlen = 2;}
		len += (fieldlen+8);
	}
	buffer = Tcl_Alloc(len*sizeof(char));
	/* make sql */
	fieldstring = Tcl_GetStringFromObj(objv[0],&fieldlen);
	sprintf(buffer,"update \"%s\" set \"%s\" = ?",tablestring,fieldstring);
	pos = buffer + 20 + tablelen + fieldlen;
	for (i = 2 ; i < objc ; i += 2) {
		fieldstring = Tcl_GetStringFromObj(objv[i],&fieldlen);
		sprintf(pos,", \"%s\" = ?",fieldstring);
		pos += (8 + fieldlen);
	}
	sprintf(pos," where \"id\" = '%s'",idstring);
	/* run sql */
	error = sqlite3_prepare_v2(dbdata->db,buffer,-1,&stmt,&nextsql);
	if (error != SQLITE_OK) {
		dbi_Sqlite3_Error(interp,dbdata,"preparing set statement");
		goto error;
	}
	j = 1;
	for (i = 1 ; i < objc ; i += 2) {
		dbi_Sqlite3_bindarg(interp,stmt,j,objv[i],nullstring,nulllen);
		j++;
	}
	error = sqlite3_step(stmt);
	if (error != SQLITE_DONE) {
		dbi_Sqlite3_Error(interp,dbdata,"processing set statement");
		goto error;
	}
	error = sqlite3_finalize(stmt); stmt = NULL;
	changes = sqlite3_changes(dbdata->db);
	Tcl_Free(buffer); buffer = NULL;
	switch (error) {
		case SQLITE_OK:
			break;
		default:
			Tcl_AppendResult(interp,"error setting object identified by id = '",Tcl_GetStringFromObj(id,NULL), 
			"' in table \"",	Tcl_GetStringFromObj(table,NULL), "\":\n",sqlite3_errmsg(dbdata->db), NULL);
			goto error;
	}
	if (changes != 1) {
		if (buffer != NULL) {Tcl_Free(buffer);}
		if (stmt != NULL) {sqlite3_finalize(stmt);}
		return dbi_Sqlite3_Insert(interp,dbdata,table,id,NULL,objc,objv);
	}
	dbi_Sqlite3_ClearResult(dbdata);
	return TCL_OK;
	error:
		if (buffer != NULL) {Tcl_Free(buffer);}
		if (stmt != NULL) {sqlite3_finalize(stmt);}
		dbi_Sqlite3_ClearResult(dbdata);
		return TCL_ERROR;
}

int dbi_Sqlite3_Unset(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	Tcl_Obj *object,
	int objc,
	Tcl_Obj **objv)
{
	sqlite3_stmt *stmt = NULL;
	const char *nextsql;
	Tcl_Obj *table,*id;
	char *buffer = NULL,*idstring,*tablestring,*fieldstring,*pos;
	int i,error,len,idlen,tablelen,fieldlen,changes;
	if (objc == 1) {
		error = Tcl_ListObjGetElements(interp,objv[0],&objc,&objv);
		if (error) {return TCL_ERROR;}
	}
	if (objc == 0) {
		Tcl_AppendResult(interp,"no fields given to unset", NULL);
		goto error;
	}
	error = dbi_Sqlite3_SplitObject(interp,object,&table,&id);
	if (error) {goto error;}
	tablestring = Tcl_GetStringFromObj(table,&tablelen);
	idstring = Tcl_GetStringFromObj(id,&idlen);
	len = 36 + tablelen + idlen;
	for (i = 0 ; i < objc ; i++) {
		Tcl_GetStringFromObj(objv[i],&fieldlen);	
		len += (fieldlen+11);
	}
	buffer = Tcl_Alloc(len*sizeof(char));
	fieldstring = Tcl_GetStringFromObj(objv[0],&fieldlen);
	sprintf(buffer,"update \"%s\" set \"%s\" = NULL",tablestring,fieldstring);
	pos = buffer + 23 + tablelen + fieldlen;
	for (i = 1 ; i < objc ; i++) {
		fieldstring = Tcl_GetStringFromObj(objv[i],&fieldlen);
		sprintf(pos,", \"%s\" = NULL",fieldstring);
		pos += (11 + fieldlen);
	}
	sprintf(pos," where \"id\" = '%s'",idstring);
	error = sqlite3_prepare_v2(dbdata->db,buffer,-1,&stmt,&nextsql);
	if (error != SQLITE_OK) {
		dbi_Sqlite3_Error(interp,dbdata,"preparing delete statement");
		goto error;
	}
	error = sqlite3_step(stmt);
	if (error != SQLITE_DONE) {
		dbi_Sqlite3_Error(interp,dbdata,"processing delete statement");
		goto error;
	}
	error = sqlite3_finalize(stmt); stmt = NULL;
	changes = sqlite3_changes(dbdata->db);
	Tcl_Free(buffer); buffer = NULL;
	switch (error) {
		case SQLITE_OK:
			break;
		default:
			Tcl_AppendResult(interp,"error setting object identified by id = '",Tcl_GetStringFromObj(id,NULL), 
			"' in table \"",	Tcl_GetStringFromObj(table,NULL), "\":\n",sqlite3_errmsg(dbdata->db), NULL);
			goto error;
	}
	if (changes != 1) {
		Tcl_AppendResult(interp,"object {",Tcl_GetStringFromObj(object,NULL),"} not found",NULL); 
		goto error;
	}
	return TCL_OK;
	error:
		if (buffer != NULL) {Tcl_Free(buffer);}
		if (stmt != NULL) {sqlite3_finalize(stmt);}
		dbi_Sqlite3_ClearResult(dbdata);
		return TCL_ERROR;
}

int dbi_Sqlite3_Delete(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	Tcl_Obj *object)
{
	sqlite3_stmt *stmt = NULL;
	const char *nextsql;
	Tcl_Obj *table,*id;
	char *buffer = NULL,*idstring,*tablestring;
	int error,len,idlen,tablelen,changes;
	error = dbi_Sqlite3_SplitObject(interp,object,&table,&id);
	if (error) {goto error;}
	tablestring = Tcl_GetStringFromObj(table,&tablelen);
	idstring = Tcl_GetStringFromObj(id,&idlen);
	len = 31 + tablelen + idlen;
	buffer = Tcl_Alloc(len*sizeof(char));
	sprintf(buffer,"delete from \"%s\" where \"id\" = '%s'",tablestring,idstring);
	error = sqlite3_prepare_v2(dbdata->db,buffer,-1,&stmt,&nextsql);
	if (error != SQLITE_OK) {
		dbi_Sqlite3_Error(interp,dbdata,"preparing delete statement");
		goto error;
	}
	error = sqlite3_step(stmt);
	if (error != SQLITE_DONE) {
		dbi_Sqlite3_Error(interp,dbdata,"processing delete statement");
		goto error;
	}
	error = sqlite3_finalize(stmt); stmt = NULL;
	changes = sqlite3_changes(dbdata->db);
	Tcl_Free(buffer); buffer = NULL;
	switch (error) {
		case SQLITE_OK:
			break;
		default:
			Tcl_AppendResult(interp,"error deleting object identified by id = '",Tcl_GetStringFromObj(id,NULL), 
			"' in table \"",	Tcl_GetStringFromObj(table,NULL), "\":\n",sqlite3_errmsg(dbdata->db), NULL);
			goto error;
	}
	if (changes != 1) {
		Tcl_AppendResult(interp,"object {",Tcl_GetStringFromObj(object,NULL), "} not found", NULL);
		goto error;
	}
	dbi_Sqlite3_ClearResult(dbdata);
	return TCL_OK;
	error:
		if (buffer != NULL) {Tcl_Free(buffer);}
		if (stmt != NULL) {sqlite3_finalize(stmt);}
		dbi_Sqlite3_ClearResult(dbdata);
		return TCL_ERROR;
}

int dbi_Sqlite3_getrow(
	Tcl_Interp *interp,
	sqlite3_stmt *stmt,
	Tcl_Obj *nullvalue,
	Tcl_Obj **result)
{
	Tcl_Obj *line = NULL, *pVal = NULL;
	int i,error,nCol;
	if (stmt == NULL) {
		Tcl_AppendResult(interp,"error in getrow: empty stmt", NULL);
		return TCL_ERROR;
	}
	nCol = sqlite3_column_count(stmt);
	line = Tcl_NewObj();
	for (i = 0; i < nCol ; i++) {
		error = dbi_Sqlite3_getresultfield(interp,stmt,i,&pVal);
		if (error) {goto error;}
		if (pVal != NULL) {
			error = Tcl_ListObjAppendElement(interp,line,pVal);
			if (error) {goto error;}
		} else {
			error = Tcl_ListObjAppendElement(interp,line,nullvalue);
			if (error) {goto error;}
		}
		pVal = NULL;
	}
	*result = line;
	return TCL_OK;
	error:
		if (line != NULL) {Tcl_DecrRefCount(line);}
		if (pVal != NULL) {Tcl_DecrRefCount(pVal);}
		return TCL_ERROR;
}

int dbi_Sqlite3_preparenext(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	sqlite3_stmt **stmtPtr,
	char **sql)
{
	sqlite3_stmt *stmt=NULL;
	const char *nextsql;
	int error;
	*stmtPtr = NULL;
	nextsql = *sql;
	while (stmt == NULL) {
		error = sqlite3_prepare_v2(dbdata->db,nextsql,-1,&stmt,&nextsql);
		if (error != SQLITE_OK) {
			sqlite3_finalize(stmt); stmt = NULL;
			dbi_Sqlite3_Error(interp,dbdata,"preparing statement");
			return TCL_ERROR;
		}
		if (*nextsql == '\0') break;
	}
	*sql = (char *)nextsql;
	*stmtPtr = stmt;
	return TCL_OK;
}

int dbi_Sqlite3_Exec(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	Tcl_Obj *cmd,
	int flags,
	Tcl_Obj *nullvalue,
	int objc,
	Tcl_Obj **objv)
{
	Tcl_Obj *line;
	sqlite3_stmt *stmt=NULL;
	char *cmdstring = NULL, *nullstring;
	char *nextsql = NULL;
	int error,error2,cmdlen,nulllen;
	int usefetch,changes=-1,cols=0;
	int numargs,i;
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
	dbi_Sqlite3_ClearResult(dbdata);
	cmdstring = Tcl_GetStringFromObj(cmd,&cmdlen);
	nextsql = cmdstring;
	error = dbi_Sqlite3_preparenext(interp,dbdata,&(dbdata->stmt),&nextsql);
	if (error) {
		Tcl_AppendResult(interp," while executing command: \"",	cmdstring, "\"", NULL);
		return TCL_ERROR;
	}
	numargs = sqlite3_bind_parameter_count(dbdata->stmt);
	if ((numargs != 0) && (*nextsql != '\0')) {
		error = dbi_Sqlite3_preparenext(interp,dbdata,&(dbdata->stmt),&nextsql);
		if (dbdata->stmt != NULL) {
			sqlite3_finalize(dbdata->stmt); dbdata->stmt = NULL;
			Tcl_AppendResult(interp,"only one SQL command can be given when parameters are used", NULL);
			Tcl_AppendResult(interp," while executing command: \"",	cmdstring, "\"", NULL);
			return TCL_ERROR;
		}
	}
	if (objc != numargs) {
		sqlite3_finalize(dbdata->stmt); dbdata->stmt = NULL;
		Tcl_AppendResult(interp,"wrong number of arguments given to exec", NULL);
		Tcl_AppendResult(interp," while executing command: \"",	cmdstring, "\"", NULL);
		return TCL_ERROR;
	}
	if (nullvalue == NULL) {
		nullstring = NULL; nulllen = 0;
		dbdata->nullvalue = dbdata->defnullvalue;
	} else {
		nullstring = Tcl_GetStringFromObj(nullvalue,&nulllen);
		dbdata->nullvalue = nullvalue;
	}
	if (usefetch) {
		dbdata->nullvalue = dbdata->defnullvalue;
	}
	if (numargs > 0) {
		/* the SQL takes arguments */
		for(i = 1; i <= numargs; i++){
			dbi_Sqlite3_bindarg(interp,dbdata->stmt,i,objv[i-1],nullstring,nulllen);
		}
		error = sqlite3_step(dbdata->stmt);
		if ((error == SQLITE_ERROR) || (error == SQLITE_MISUSE)) {
			error = sqlite3_finalize(dbdata->stmt); dbdata->stmt = NULL;
		}
	} else {
		/* no arguments, multiple commands are possible */
		int toclose=0;
		if (*nextsql != '\0') {
			error = dbi_Sqlite3_Transaction_Start(interp,dbdata);
/*fprintf(stdout,"transaction start:%s: %d\n%s\n",nextsql,error,dbdata->errormsg);fflush(stdout);*/
			if (error) {
				if (strcmp("cannot start a transaction within a transaction",dbdata->errormsg) == 0) {
					sqlite3_free(dbdata->errormsg); dbdata->errormsg=NULL;
					toclose = 0;
				} else {
					dbi_Sqlite3_ClearResult(dbdata); return TCL_ERROR;
				}
			} else {
				toclose = 1;
			}
		}
		while (1) {
			error = sqlite3_step(dbdata->stmt);
			if ((error == SQLITE_ERROR) || (error == SQLITE_MISUSE)) {
				error = sqlite3_finalize(dbdata->stmt); dbdata->stmt = NULL;
				break;
			}
			error2 = dbi_Sqlite3_preparenext(interp,dbdata,&stmt,&nextsql);
			if (error2) {
				sqlite3_finalize(dbdata->stmt); dbdata->stmt = NULL;
				Tcl_AppendResult(interp," while executing command: \"",	cmdstring, "\"", NULL);
				if (toclose) {
					error = dbi_Sqlite3_Transaction_Rollback(interp,dbdata);
					if (error) {dbi_Sqlite3_Error(interp,dbdata,"autorollback transaction");}
				}
				return TCL_ERROR;
			}
			if (stmt == NULL) {
				break;
			} else {
				error2 = sqlite3_finalize(dbdata->stmt);
				dbdata->stmt = stmt;
				if (error2) {
					if (toclose) {
						error = dbi_Sqlite3_Transaction_Rollback(interp,dbdata);
						if (error) {dbi_Sqlite3_Error(interp,dbdata,"autorollback transaction");}
					}
					return TCL_ERROR;
				}
			}
		}
		if (toclose) {
			error2 = dbi_Sqlite3_Transaction_Commit(interp,dbdata,1);
			if (error2) {
				dbi_Sqlite3_ClearResult(dbdata);
				dbi_Sqlite3_Error(interp,dbdata,"autocommitting transaction");return error2;
			}
		}
	}
	switch (error) {
		case SQLITE_DONE:
			cols = sqlite3_column_count(dbdata->stmt);
			changes = sqlite3_changes(dbdata->db);
			error2 = dbi_Sqlite3_getcolnames(interp,dbdata);
			if (error2) {goto error;}
			dbdata->result = Tcl_NewListObj(0,NULL);
			break;
		case SQLITE_ROW:
			cols = sqlite3_column_count(dbdata->stmt);
			error2 = dbi_Sqlite3_getcolnames(interp,dbdata);
			if (error2) {goto error;}
			dbdata->result = Tcl_NewListObj(0,NULL);
			while (error == SQLITE_ROW) {
				error = dbi_Sqlite3_getrow(interp,dbdata->stmt,dbdata->nullvalue,&line);
				if (error) {
					dbi_Sqlite3_ClearResult(dbdata); return error;
				}
				if (dbdata->resultflat) {
					error = Tcl_ListObjAppendList(interp,dbdata->result,line);
					Tcl_DecrRefCount(line);
				} else {
					error = Tcl_ListObjAppendElement(interp,dbdata->result,line);
				}
				if (error) {
					Tcl_DecrRefCount(line);
					dbi_Sqlite3_ClearResult(dbdata);
					return error;
				}
				error = sqlite3_step(dbdata->stmt);
			}
			if (error != SQLITE_DONE) {
				dbi_Sqlite3_ClearResult(dbdata);return error;
			}
			error = sqlite3_finalize(dbdata->stmt);dbdata->stmt = NULL;
			if (error) {Tcl_DecrRefCount(dbdata->result);dbdata->result=NULL;return error;}
			break;
		default:
			sqlite3_finalize(dbdata->stmt); dbdata->stmt = NULL;
			Tcl_AppendResult(interp,"database error executing command \"",
				cmdstring, "\":\n",	sqlite3_errmsg(dbdata->db), NULL);
			goto error;
	}
	if (!usefetch) {
		if (cols != 0) {
			Tcl_SetObjResult(interp,dbdata->result);
			dbdata->result = NULL;
		} else if (changes != -1) {
			Tcl_SetObjResult(interp,Tcl_NewIntObj(changes));
			dbdata->tuple = -1;
		}
		dbi_Sqlite3_ClearResult(dbdata);
	} else {
		dbdata->tuple = -1;
	}
	return TCL_OK;
	error:
		dbi_Sqlite3_ClearResult(dbdata);
		return TCL_ERROR;
}

int dbi_Sqlite3_Supports(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	Tcl_Obj *keyword)
{
	static CONST char *keywords[] = {
		"lines","backfetch","serials","sharedserials","blobparams","blobids",
		"transactions","sharedtransactions","foreignkeys","checks","views",
		"columnperm","roles","domains","permissions",
		(char *) NULL};
	static CONST int supports[] = {
		1,1,1,1,0,0,
		1,1,1,1,1,
		0,0,0,0};
	enum keywordsIdx {
		Lines,Backfetch,Serials,Sharedserials,Blobparams, Blobids,
		Transactions,Sharedtransactions,Foreignkeys,Checks,Views,
		Columnperm, Roles, Domains, Permissions
	};
	int error,index;
	if (keyword == NULL) {
		char CONST **keyword = keywords;
		int CONST *value = supports;
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

int dbi_Sqlite3_Info(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	int objc,
	Tcl_Obj **objv)
{
	Tcl_Obj *cmd,*dbcmd;
	int error,i;
	dbcmd = Tcl_NewObj();
	Tcl_GetCommandFullName(interp, dbdata->token, dbcmd);
	cmd = Tcl_NewStringObj("::dbi::sqlite3::info",-1);
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

int dbi_Sqlite3_Tables(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata)
{
	int error;
	Tcl_Obj *cmd = Tcl_NewStringObj("SELECT name FROM sqlite_master WHERE type in ('table','view') and name not like '_dbi_%' ORDER BY name",-1);
	Tcl_IncrRefCount(cmd);
	error = dbi_Sqlite3_Exec(interp,dbdata,cmd,EXEC_FLAT,NULL,0,NULL);
	Tcl_DecrRefCount(cmd);
	return error;
}

int dbi_Sqlite3_Cmd(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
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

int dbi_Sqlite3_Serial(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	Tcl_Obj *subcmd,
	int objc,
	Tcl_Obj **objv)
{
	int error,index;
	static CONST char *subCmds[] = {
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
			error = dbi_Sqlite3_TclEval(interp,dbdata,"::dbi::sqlite3::serial_add",objc,objv);	
			break;
		case Delete:
			error = dbi_Sqlite3_TclEval(interp,dbdata,"::dbi::sqlite3::serial_delete",objc,objv);	
			break;
		case Set:
			error = dbi_Sqlite3_TclEval(interp,dbdata,"::dbi::sqlite3::serial_set",objc,objv);	
			break;
		case Next:
			error = dbi_Sqlite3_TclEval(interp,dbdata,"::dbi::sqlite3::serial_next",objc,objv);	
			break;
		case Share:
			error = dbi_Sqlite3_TclEval(interp,dbdata,"::dbi::sqlite3::serial_share",objc,objv);	
			break;
	}
	return error;
}

int dbi_Sqlite3_Createdb(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
	static CONST char *switches[] = {"-sync", (char *) NULL};
	enum switchesIdx {Sync};
	char *syncstring = "PRAGMA default_synchronous = OFF";
	int i,index,error;
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
	error = sqlite3_open(Tcl_GetStringFromObj(dbdata->database,NULL), &(dbdata->db));
	if (error != SQLITE_OK) {
		Tcl_AppendResult(interp,"connection to database \"",
			Tcl_GetStringFromObj(dbdata->database,NULL) , "\" failed:\n", sqlite3_errmsg(dbdata->db), NULL);
		sqlite3_close(dbdata->db);
		Tcl_DecrRefCount(dbdata->database);dbdata->database=NULL;
		goto error;
	}
	for (i = 3; i < objc; i++) {
		if (Tcl_GetIndexFromObj(interp, objv[i], switches, "option", 0, &index)!= TCL_OK) {
			sqlite3_close(dbdata->db);
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
	error = sqlite3_exec(dbdata->db,syncstring,NULL,dbdata,&(dbdata->errormsg));
	if (error) {sqlite3_free(dbdata->errormsg); dbdata->errormsg=NULL;}
	sqlite3_close(dbdata->db);
	dbdata->db = NULL;
	return TCL_OK;
	error:
		return TCL_ERROR;
}

int dbi_Sqlite3_Close(
	dbi_Sqlite3_Data *dbdata)
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
		dbi_Sqlite3_ClearResult(dbdata);
		if (dbdata->db != NULL) {
			sqlite3_close(dbdata->db);
			dbdata->db = NULL;
		}
	} else {
		/* this is a clone, so remove the clone from its parent */
		dbi_Sqlite3_Data *parent = dbdata->parent;
		int i;
		for (i = 0 ; i < parent->clonesnum; i++) {
			if (parent->clones[i] == dbdata) break;
		}
		i++;
		for (; i < parent->clonesnum; i++) {
			parent->clones[i-1] = parent->clones[i];
		}
		parent->clonesnum--;
		dbi_Sqlite3_ClearResult(dbdata);
	}
	return TCL_OK;
}

int dbi_Sqlite3_Dropdb(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata)
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
	error = dbi_Sqlite3_Close(dbdata);
	if (error) {Tcl_DecrRefCount(database);return error;}
	error = Tcl_VarEval(interp,"file delete ",Tcl_GetStringFromObj(database,NULL),NULL);
	Tcl_DecrRefCount(database);
	if (error) {return error;}
	return TCL_OK;
}

int dbi_Sqlite3_Destroy(
	ClientData clientdata)
{
	dbi_Sqlite3_Data *dbdata = (dbi_Sqlite3_Data *)clientdata;
	if (dbdata->database != NULL) {
		dbi_Sqlite3_Close(dbdata);
	}
	if (dbdata->parent == NULL) {
		/* this is not a clone, so really remove defnullvalue */
		Tcl_DecrRefCount(dbdata->defnullvalue);
	}
	while( dbdata->pFunc ){
		SqlFunc *pFunc = dbdata->pFunc;
		dbdata->pFunc = pFunc->pNext;
		Tcl_DecrRefCount(pFunc->pScript);
		Tcl_Free((char*)pFunc);
	}
	while( dbdata->pCollate ){
		SqlCollate *pCollate = dbdata->pCollate;
		dbdata->pCollate = pCollate->pNext;
		Tcl_Free((char*)pCollate);
	}
	Tcl_Free((char *)dbdata);
	Tcl_DeleteExitHandler((Tcl_ExitProc *)dbi_Sqlite3_Destroy, clientdata);
	return TCL_OK;
}

int dbi_Sqlite3_Interface(
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	int i;
	static CONST char *interfaces[] = {
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

int Dbi_sqlite3_DbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_Sqlite3_Data *dbdata = (dbi_Sqlite3_Data *)clientdata;
	int error=TCL_OK,i,index;
	static CONST char *subCmds[] = {
		"interface","open", "exec", "fetch", "close",
		"info", "tables","fields",
		"begin", "commit", "rollback",
		"destroy", "serial","supports",
		"create", "drop","clone","clones","parent",
		"get","set","unset","insert","delete",
		"function","collate",
		(char *) NULL};
	enum ISubCmdIdx {
		Interface, Open, Exec, Fetch, Close,
		Info, Tables, Fields,
		Begin, Commit, Rollback,
		Destroy, Serial, Supports,
		Create, Drop, Clone, Clones, Parent,
		Get,Set,Unset,Insert,Delete,
		Function,Collate
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
		return dbi_Sqlite3_Interface(interp,objc,objv);
	case Open:
		if (dbdata->parent != NULL) {
			Tcl_AppendResult(interp,"clone may not use open",NULL);
			return TCL_ERROR;
		}
		return dbi_Sqlite3_Open(interp,dbdata,objc,objv);
	case Create:
		return dbi_Sqlite3_Createdb(interp,dbdata,objc,objv);
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
				if (strncmp(Tcl_GetStringFromObj(dbcmd,NULL),"::dbi::sqlite3::priv_",23) == 0) continue;
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
		static CONST char *switches[] = {"-usefetch", "-nullvalue", "-flat","-cache", (char *) NULL};
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
		return dbi_Sqlite3_Exec(interp,dbdata,objv[i],flags,nullvalue,objc-i-1,objv+i+1);
		}
	case Fetch:
		return dbi_Sqlite3_Fetch(interp,dbdata, objc, objv);
	case Info:
		return dbi_Sqlite3_Info(interp,dbdata,objc-2,objv+2);
	case Tables:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_Sqlite3_Tables(interp,dbdata);
	case Fields:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "tablename");
			return TCL_ERROR;
		}
		return dbi_Sqlite3_Cmd(interp,dbdata,objv[2],NULL,"::dbi::sqlite3::fieldsinfo");
	case Close:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		if (dbdata->parent == NULL) {
			return dbi_Sqlite3_Close(dbdata);
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
		error = dbi_Sqlite3_Transaction_Commit(interp,dbdata,1);
		if (error) {sqlite3_free(dbdata->errormsg); dbdata->errormsg=NULL;}
		error = dbi_Sqlite3_Transaction_Start(interp,dbdata);
		if (error) {dbi_Sqlite3_Error(interp,dbdata,"starting transaction");return error;}
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
		error = dbi_Sqlite3_Transaction_Commit(interp,dbdata,1);
		if (error) {dbi_Sqlite3_Error(interp,dbdata,"committing transaction");return error;}
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
		error = dbi_Sqlite3_Transaction_Rollback(interp,dbdata);
		if (error) {dbi_Sqlite3_Error(interp,dbdata,"rolling back transaction");return error;}
		return TCL_OK;
	case Serial:
		if (objc < 5) {
			Tcl_WrongNumArgs(interp, 2, objv, "option table field ?value?");
			return TCL_ERROR;
		}
		return dbi_Sqlite3_Serial(interp,dbdata,objv[2],objc-3,objv+3);
	case Drop:
		if (objc != 2) {
			Tcl_WrongNumArgs(interp, 2, objv, "");
			return TCL_ERROR;
		}
		return dbi_Sqlite3_Dropdb(interp,dbdata);
	case Clone:
		if ((objc != 2) && (objc != 3)) {
			Tcl_WrongNumArgs(interp, 2, objv, "?name?");
			return TCL_ERROR;
		}
		if (objc == 3) {
			return Dbi_sqlite3_Clone(interp,dbdata,objv[2]);
		} else {
			return Dbi_sqlite3_Clone(interp,dbdata,NULL);
		}
	case Supports:
		if (objc == 2) {
			return dbi_Sqlite3_Supports(interp,dbdata,NULL);
		} if (objc == 3) {
			return dbi_Sqlite3_Supports(interp,dbdata,objv[2]);
		} else {
			Tcl_WrongNumArgs(interp, 2, objv, "?keyword?");
			return TCL_ERROR;
		}
	case Get:
		if (objc < 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "{table id} ?field ...?");
			return TCL_ERROR;
		}
		return dbi_Sqlite3_Get(interp,dbdata,objv[2],objc-3,objv+3);
	case Set:
		{
		static CONST char *switches[] = {"-nullvalue", (char *) NULL};
		enum switchesIdx {Nullvalue};
		Tcl_Obj *nullvalue = NULL;
		char *string;
		if (objc < 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "?options? {table id} list | field value ?field value?");
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
				case Nullvalue:
					i++;
					if (i == objc) {
						Tcl_AppendResult(interp,"\"-nullvalue\" option must be followed by the value returned for NULL columns",NULL);
						return TCL_ERROR;
					}
					nullvalue = objv[i];
					break;
			}
			i++;
		}
		return dbi_Sqlite3_Set(interp,dbdata,objv[i],nullvalue,objc-i-1,objv+i+1);
		}
	case Insert:
		{
		static CONST char *switches[] = {"-nullvalue", (char *) NULL};
		enum switchesIdx {Nullvalue};
		Tcl_Obj *nullvalue = NULL;
		char *string;
		if (objc < 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "?options? table list | field value ?field value?");
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
				case Nullvalue:
					i++;
					if (i == objc) {
						Tcl_AppendResult(interp,"\"-nullvalue\" option must be followed by the value returned for NULL columns",NULL);
						return TCL_ERROR;
					}
					nullvalue = objv[i];
					break;
			}
			i++;
		}
		return dbi_Sqlite3_Insert(interp,dbdata,objv[i],NULL,nullvalue,objc-i-1,objv+i+1);
		}
	case Unset:
		if (objc < 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "{table id} field ?field ...?");
			return TCL_ERROR;
		}
		return dbi_Sqlite3_Unset(interp,dbdata,objv[2],objc-3,objv+3);
	case Delete:
		if (objc != 3) {
			Tcl_WrongNumArgs(interp, 2, objv, "{table id}");
			return TCL_ERROR;
		}
		return dbi_Sqlite3_Delete(interp,dbdata,objv[2]);
	case Function:
		{
		SqlFunc *pFunc;
		Tcl_Obj *pScript;
		char *zName;
		if (objc!=4) {
			Tcl_WrongNumArgs(interp, 2, objv, "name script");
			return TCL_ERROR;
		}
		zName = Tcl_GetStringFromObj(objv[2], 0);
		pScript = objv[3];
		pFunc = findSqlFunc(dbdata, zName);
		if (pFunc==0) return TCL_ERROR;
		if (pFunc->pScript) {
			Tcl_DecrRefCount(pFunc->pScript);
		}
		pFunc->pScript = pScript;
		Tcl_IncrRefCount(pScript);
		pFunc->useEvalObjv = safeToUseEvalObjv(interp, pScript);
		error = sqlite3_create_function(dbdata->db, zName, -1, SQLITE_UTF8,
			  pFunc, Dbi_sqlite3_tclSqlFunc, 0, 0);
		if( error!=SQLITE_OK ){
			error = TCL_ERROR;
			Tcl_SetResult(interp, (char *)sqlite3_errmsg(dbdata->db), TCL_VOLATILE);
		}else{
			/* Must flush any cached statements */
			/*flushStmtCache( pDb );*/
		}
		break;
		}
	case Collate:
		{
		SqlCollate *pCollate;
		char *zName;
		char *zScript;
		int nScript;
		if( objc!=4 ){
			Tcl_WrongNumArgs(interp, 2, objv, "name script");
			return TCL_ERROR;
		}
		zName = Tcl_GetStringFromObj(objv[2], 0);
		zScript = Tcl_GetStringFromObj(objv[3], &nScript);
		pCollate = (SqlCollate*)Tcl_Alloc( sizeof(*pCollate) + nScript + 1 );
		if( pCollate==0 ) return TCL_ERROR;
		pCollate->interp = interp;
		pCollate->pNext = dbdata->pCollate;
		pCollate->zScript = (char*)&pCollate[1];
		dbdata->pCollate = pCollate;
		strcpy(pCollate->zScript, zScript);
		if( sqlite3_create_collation(dbdata->db, zName, SQLITE_UTF8, pCollate, Dbi_sqlite3_tclSqlCollate) ){
			Tcl_SetResult(interp, (char *)sqlite3_errmsg(dbdata->db), TCL_VOLATILE);
			return TCL_ERROR;
		}
		break;
		}
	}
	return error;
}

static int dbi_num = 0;
int Dbi_sqlite3_DoNewDbObjCmd(
	dbi_Sqlite3_Data *dbdata,
	Tcl_Interp *interp,
	Tcl_Obj *dbi_nameObj)
{
	char buffer[40];
	char *dbi_name;
	dbdata->interp = interp;
	dbdata->db = NULL;
	dbdata->stmt = NULL;
	dbdata->database=NULL;
	dbdata->defnullvalue = NULL;
	dbdata->result = NULL;
	dbdata->resultfields = NULL;
	dbdata->resultflat = 0;
	dbdata->nullvalue = NULL;
	dbdata->clones = NULL;
	dbdata->clonesnum = 0;
	dbdata->parent = NULL;
	dbdata->errormsg = NULL;
	dbdata->pFunc = NULL;
	dbdata->pCollate = NULL;
	if (dbi_nameObj == NULL) {
		dbi_num++;
		sprintf(buffer,"::dbi::sqlite3::dbi%d",dbi_num);
		dbi_nameObj = Tcl_NewStringObj(buffer,strlen(buffer));
	}
	dbi_name = Tcl_GetStringFromObj(dbi_nameObj,NULL);
	dbdata->token = Tcl_CreateObjCommand(interp,dbi_name,(Tcl_ObjCmdProc *)Dbi_sqlite3_DbObjCmd,
		(ClientData)dbdata,(Tcl_CmdDeleteProc *)dbi_Sqlite3_Destroy);
	Tcl_CreateExitHandler((Tcl_ExitProc *)dbi_Sqlite3_Destroy, (ClientData)dbdata);
	Tcl_SetObjResult(interp,dbi_nameObj);
	return TCL_OK;
}

int Dbi_sqlite3_NewDbObjCmd(
	ClientData clientdata,
	Tcl_Interp *interp,
	int objc,
	Tcl_Obj *objv[])
{
	dbi_Sqlite3_Data *dbdata;
	int error;
	if ((objc < 1)||(objc > 2)) {
		Tcl_WrongNumArgs(interp,2,objv,"?dbName?");
		return TCL_ERROR;
	}
	dbdata = (dbi_Sqlite3_Data *)Tcl_Alloc(sizeof(dbi_Sqlite3_Data));
	if (objc == 2) {
		error = Dbi_sqlite3_DoNewDbObjCmd(dbdata,interp,objv[1]);
	} else {
		error = Dbi_sqlite3_DoNewDbObjCmd(dbdata,interp,NULL);
	}
	dbdata->defnullvalue = Tcl_NewObj();
	Tcl_IncrRefCount(dbdata->defnullvalue);
	if (error) {
		Tcl_Free((char *)dbdata);
	}
	return error;
}

int Dbi_sqlite3_Clone(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	Tcl_Obj *name)
{
	dbi_Sqlite3_Data *parent = NULL;
	dbi_Sqlite3_Data *clone_dbdata = NULL;
	int error;
	parent = dbdata;
	while (parent->parent != NULL) {parent = parent->parent;}
	clone_dbdata = (dbi_Sqlite3_Data *)Tcl_Alloc(sizeof(dbi_Sqlite3_Data));
	error = Dbi_sqlite3_DoNewDbObjCmd(clone_dbdata,interp,name);
	if (error) {Tcl_Free((char *)clone_dbdata);return TCL_ERROR;}
	parent->clonesnum++;
	parent->clones = (struct dbi_Sqlite3_Data **)Tcl_Realloc((char *)parent->clones,parent->clonesnum*sizeof(dbi_Sqlite3_Data **));
	parent->clones[parent->clonesnum-1] = clone_dbdata;
	clone_dbdata->parent = parent;
	clone_dbdata->db = parent->db;
	clone_dbdata->stmt = NULL;
	clone_dbdata->database = parent->database;
	clone_dbdata->defnullvalue = parent->defnullvalue;
	clone_dbdata->result = NULL;
	clone_dbdata->resultfields = NULL;
	clone_dbdata->resultflat = 0;
	clone_dbdata->nullvalue = NULL;
	clone_dbdata->errormsg = NULL;
	clone_dbdata->stmt = NULL;
	clone_dbdata->pFunc = NULL;
	clone_dbdata->pCollate = NULL;
	return TCL_OK;
}

int Dbi_sqlite3_Init(interp)
	Tcl_Interp *interp;		/* Interpreter to add extra commands */
{
#ifdef USE_TCL_STUBS
	if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
		return TCL_ERROR;
	}
#endif
	Tcl_CreateObjCommand(interp,"dbi_sqlite3",(Tcl_ObjCmdProc *)Dbi_sqlite3_NewDbObjCmd,
		(ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
	Tcl_Eval(interp,"");
	return TCL_OK;
}
