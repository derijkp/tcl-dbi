/* $Format: "#define DBI_VERSION \"$ProjectMajorVersion$.$ProjectMinorVersion$\""$ */
#define DBI_VERSION "1.0"

#include <sqlite3.h>
#include "tcl.h"
/* #include "tclInt.h"*/

#define SEGM_SIZE 8192

typedef struct dbi_Sqlite3_Data {
	Tcl_Command token;
	Tcl_Interp *interp;
	sqlite3 *db;
	sqlite3_stmt *stmt;
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
	int autocommit;
	int intransaction;
	char *errormsg;
	struct dbi_Sqlite3_Data *parent;
	struct dbi_Sqlite3_Data **clones;
	int clonesnum;
} dbi_Sqlite3_Data;

EXTERN int dbi_sqlite3_Init(Tcl_Interp *interp);

#define EXEC_USEFETCH 1
#define EXEC_FLAT 2
#define EXEC_BLOBID 4
#define EXEC_CACHE 4

#define DBI_DOUBLE 'D'
#define DBI_INT 'I'
#define DBI_TEXT 'T'
