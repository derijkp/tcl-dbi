#include "tcl.h"
#include "dbi.h"
#include <sys/types.h>
#include <time.h>
#include <math.h>

int dbi_NewDbObjCmd(ClientData clientdata,Tcl_Interp *interp,int objc,Tcl_Obj *objv[]);

/* dbi_TypeOpen dbi_MixedOpen; */
dbi_TypeCreate dbi_Postgresql_Create;

int
Dbi_Init(interp)
	Tcl_Interp *interp;		/* Interpreter to add extra commands */
{
	Tcl_CreateObjCommand(interp,"dbi",(Tcl_ObjCmdProc *)dbi_NewDbObjCmd,
		(ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
	Dbi_Postgresql_Init(interp);
	Dbi_Odbc_Init(interp);
	return TCL_OK;
}


