#include "tcl.h"
#include "dbi.h"
#include <sys/types.h>
#include <time.h>
#include <math.h>

int dbi_NewDbObjCmd(ClientData clientdata,Tcl_Interp *interp,int objc,Tcl_Obj *objv[]);

/* dbi_TypeOpen dbi_MixedOpen; */
dbi_TypeCreate dbi_Postgresql_Create;

int
dbi_Init(interp)
	Tcl_Interp *interp;		/* Interpreter to add extra commands */
{
	Tcl_CreateObjCommand(interp,"dbi",(Tcl_ObjCmdProc *)dbi_NewDbObjCmd,
		(ClientData)NULL,(Tcl_CmdDeleteProc *)NULL);
	dbi_CreateType(interp,"postgresql",dbi_Postgresql_Create);
	return TCL_OK;
}


