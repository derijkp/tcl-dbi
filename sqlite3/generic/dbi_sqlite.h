/* $Format: "#define DBI_VERSION \"$ProjectMajorVersion$.$ProjectMinorVersion$\""$ */
#define DBI_VERSION "1.0"

#include "sqlite3.h"
#include "tcl.h"
/* #include "tclInt.h"*/

#define SEGM_SIZE 8192

/*
** New SQL functions can be created as TCL scripts.  Each such function
** is described by an instance of the following structure.
*/
typedef struct SqlFunc SqlFunc;
struct SqlFunc {
  Tcl_Interp *interp;   /* The TCL interpret to execute the function */
  Tcl_Obj *pScript;     /* The Tcl_Obj representation of the script */
  int useEvalObjv;      /* True if it is safe to use Tcl_EvalObjv */
  char *zName;          /* Name of this function */
  SqlFunc *pNext;       /* Next function on the list of them all */
};

/*
** New collation sequences function can be created as TCL scripts.  Each such
** function is described by an instance of the following structure.
*/
typedef struct SqlCollate SqlCollate;
struct SqlCollate {
  Tcl_Interp *interp;   /* The TCL interpret to execute the function */
  char *zScript;        /* The script to be run */
  SqlCollate *pNext;    /* Next function on the list of them all */
};

typedef struct dbi_Sqlite3_Data {
	Tcl_Command token;
	Tcl_Interp *interp;
	sqlite3 *db;
	sqlite3_stmt *stmt;
	int cached;
	Tcl_Obj *result;
	Tcl_Obj *resultfields;
	Tcl_Obj *database;
	Tcl_Obj *nullvalue;
	Tcl_Obj *defnullvalue;
	Tcl_Obj **resultlines;
	int resultflat;
	int ntuples;
	int nfields; 
	int tuple;
	char *errormsg;
	struct dbi_Sqlite3_Data *parent;
	struct dbi_Sqlite3_Data **clones;
	int clonesnum;
	Tcl_HashTable preparedhash;
	SqlFunc *pFunc;
	SqlCollate *pCollate;
} dbi_Sqlite3_Data;

EXTERN int dbi_sqlite3_Init(Tcl_Interp *interp);

#define EXEC_USEFETCH 1
#define EXEC_FLAT 2
#define EXEC_BLOBID 4
#define EXEC_CACHE 4

#define DBI_DOUBLE 'D'
#define DBI_INT 'I'
#define DBI_TEXT 'T'
