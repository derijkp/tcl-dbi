/*	
 *	 File:    class.h
 *	 Purpose: Class extension to Tcl
 *	 Author:  Copyright (c) 1995 Peter De Rijk
 *
 *	 See the file "README" for information on usage and redistribution
 *	 of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/*
 * Windows needs to know which symbols to export.  Unix does not.
 * BUILD_Class should be undefined for Unix.
 */

#ifdef BUILD_Extral
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif /* BUILD_Extral */

typedef struct Dbi {
	Tcl_Command token;
	ClientData dbdata;
	Tcl_Interp *interp;
	char *type;
	int (*open)(Tcl_Interp *interp,struct Dbi *db,int objc,Tcl_Obj **objv);
	int (*create)(Tcl_Interp *interp,struct Dbi *db,int objc,Tcl_Obj **objv);
	int (*drop)(Tcl_Interp *interp,struct Dbi *db);
	int (*admin)(Tcl_Interp *interp,struct Dbi *db,int objc,Tcl_Obj **objv);
	int (*configure)(Tcl_Interp *interp,struct Dbi *db,Tcl_Obj *option,Tcl_Obj *value);
	int (*info)(Tcl_Interp *interp,struct Dbi *db,int objc,Tcl_Obj **objv);
	int (*tables)(Tcl_Interp *interp,struct Dbi *db);
	int (*tableinfo)(Tcl_Interp *interp,struct Dbi *db,Tcl_Obj *table,Tcl_Obj *varname,int type);
	int (*exec)(Tcl_Interp *interp,struct Dbi *db,Tcl_Obj *cmd,int usefetch,Tcl_Obj *nullvalue,int objc,Tcl_Obj **objv);
	int (*fetch)(Tcl_Interp *interp,struct Dbi *db,int flags,int line,int field,Tcl_Obj *nullvalue);
	int (*close)(Tcl_Interp *interp,struct Dbi *db);
	int (*transaction)(Tcl_Interp *interp,struct Dbi *db,int flags);
	int (*destroy)(struct Dbi *db);
	int (*serial)(Tcl_Interp *interp,struct Dbi *db,int type,Tcl_Obj *table,Tcl_Obj *field,Tcl_Obj *current);
} Dbi;

typedef int dbi_TypeCreate(Tcl_Interp *interp,Dbi *db);
int dbi_CreateType(Tcl_Interp *interp,char *type,dbi_TypeCreate (*create));

#define DBI_FETCH_DATA 0
#define DBI_FETCH_LINES 1
#define DBI_FETCH_FIELDS 2
#define DBI_FETCH_CLEAR 3
#define DBI_FETCH_ISNULL 4
#define DBI_FETCH_CURRENTLINE 5

#define TRANSACTION_BEGIN 1
#define TRANSACTION_COMMIT 2
#define TRANSACTION_ROLLBACK 3

#define SERIAL_ADD 1
#define SERIAL_DELETE 2
#define SERIAL_SET 3
#define SERIAL_NEXT 4

#define DBI_INFO_ALL 1
#define DBI_INFO_FIELDS 2

EXTERN int Dbi_Init(Tcl_Interp *interp);
