#include <libpq-fe.h>
#include "tcl.h"
#include "dbi.h"

typedef struct dbi_Postgresql_Data {
	PGconn *conn;
	PGresult *res;
	int respos;
} dbi_Postgresql_Data;

EXTERN int dbi_Postgresql_Init(Tcl_Interp *interp);
