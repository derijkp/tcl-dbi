#include <libpq-fe.h>
#include "tcl.h"
#include "dbi.h"

#define BPCHAROID 1042
#define FLOAT4OID 700
#define FLOAT8OID 701
#define TIMEOID			1083
#define TIMESTAMPOID	1184

typedef struct dbi_Postgresql_Data {
	Tcl_Command token;
	Tcl_Interp *interp;
	PGconn *conn;
	PGresult *res;
	int respos;
	struct dbi_Postgresql_Data *parent;
	struct dbi_Postgresql_Data **clones;
	int clonesnum;
} dbi_Postgresql_Data;

EXTERN int dbi_Postgresql_Init(Tcl_Interp *interp);
