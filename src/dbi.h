/*	
 *	 File:    class.h
 *	 Purpose: Class extension to Tcl
 *	 Author:  Copyright (c) 1995 Peter De Rijk
 *
 *	 See the file "README" for information on usage and redistribution
 *	 of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

typedef struct Dbi {
	Tcl_Command token;
	ClientData dbdata;
	Tcl_Interp *interp;
	int error;
	int (*open)(Tcl_Interp *interp,struct Dbi *db,int objc,Tcl_Obj **objv);
	int (*admin)(Tcl_Interp *interp,struct Dbi *db,int objc,Tcl_Obj **objv);
	int (*configure)(Tcl_Interp *interp,struct Dbi *db,Tcl_Obj *option,Tcl_Obj *value);
	int (*exec)(Tcl_Interp *interp,struct Dbi *db,Tcl_Obj *cmd);
	int (*fetch)(Tcl_Interp *interp,struct Dbi *db,Tcl_Obj *resultid,Tcl_Obj *tuple,Tcl_Obj *item);
	int (*close)(Tcl_Interp *interp,struct Dbi *db);
	int (*destroy)(struct Dbi *db);
} Dbi;

typedef int dbi_TypeCreate(Tcl_Interp *interp,Dbi *db);
