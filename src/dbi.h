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
	int (*admin)(Tcl_Interp *interp,struct Dbi *db,int objc,Tcl_Obj **objv);
	int (*configure)(Tcl_Interp *interp,struct Dbi *db,Tcl_Obj *option,Tcl_Obj *value);
	int (*tables)(Tcl_Interp *interp,struct Dbi *db);
	int (*tableinfo)(Tcl_Interp *interp,struct Dbi *db,Tcl_Obj *table,Tcl_Obj *varname);
	int (*exec)(Tcl_Interp *interp,struct Dbi *db,Tcl_Obj *cmd,int usefetch,Tcl_Obj *nullvalue);
	int (*fetch)(Tcl_Interp *interp,struct Dbi *db,int flags,int line,int field,Tcl_Obj *nullvalue);
	int (*close)(Tcl_Interp *interp,struct Dbi *db);
	int (*destroy)(struct Dbi *db);
} Dbi;

typedef int dbi_TypeCreate(Tcl_Interp *interp,Dbi *db);

#define DBI_FETCH_DATA 0
#define DBI_FETCH_LINES 1
#define DBI_FETCH_FIELDS 2
#define DBI_FETCH_CLEAR 3
#define DBI_FETCH_ISNULL 4
#define DBI_FETCH_CURRENTLINE 5

EXTERN int Dbi_Init(Tcl_Interp *interp);
