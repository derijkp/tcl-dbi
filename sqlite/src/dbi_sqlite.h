/* $Format: "#define DBI_VERSION \"0.$ProjectMajorVersion: 8 $\""$ */
#define DBI_VERSION "0.8"

#include <sqlite.h>
#include "tcl.h"

#define SEGM_SIZE 8192

typedef struct dbi_Sqlite_Data {
	Tcl_Command token;
	Tcl_Interp *interp;
	sqlite *db;
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
	char *errormsg;
	struct dbi_Sqlite_Data *parent;
	struct dbi_Sqlite_Data **clones;
	int clonesnum;
} dbi_Sqlite_Data;

EXTERN int dbi_Sqlite_Init(Tcl_Interp *interp);

#define EXEC_USEFETCH 1
#define EXEC_FLAT 2
#define EXEC_BLOBID 4
#define EXEC_CACHE 4

#define DBI_DOUBLE 'D'
#define DBI_INT 'I'
#define DBI_TEXT 'T'
