/* $Format: "#define DBI_VERSION \"$ProjectMajorVersion$.$ProjectMinorVersion$\""$ */
#define DBI_VERSION "1.0"

#include <mysql.h>
#include "tcl.h"

#define SEGM_SIZE 8192

typedef struct dbi_Mysql_Data {
	Tcl_Command token;
	Tcl_Interp *interp;
	MYSQL *db;
	MYSQL_RES *result;
	MYSQL_ROW row;
	Tcl_Obj *database;
	Tcl_Obj *nullvalue;
	Tcl_Obj *defnullvalue;
	int resultflat;
	int ntuples;
	int nfields;
	int tuple;
	char *errormsg;
	struct dbi_Mysql_Data *parent;
	struct dbi_Mysql_Data **clones;
	int clonesnum;
} dbi_Mysql_Data;

EXTERN int dbi_mysql_Init(Tcl_Interp *interp);

#define EXEC_USEFETCH 1
#define EXEC_FLAT 2
#define EXEC_BLOBID 4
#define EXEC_CACHE 4

#define DBI_DOUBLE 'D'
#define DBI_INT 'I'
#define DBI_TEXT 'T'
