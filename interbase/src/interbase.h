/* $Format: "#define DBI_VERSION \"0.$ProjectMajorVersion: 8 $\""$ */
#define DBI_VERSION "0.8"

#include <ibase.h>
#include "tcl.h"
/*#include "dbi.h"*/

typedef struct dbi_Interbase_Data {
	Tcl_Command token;
	Tcl_Interp *interp;
	isc_db_handle db;                  /* database handle */
	isc_stmt_handle stmt;              /* statement handle */
	isc_tr_handle trans;               /* transaction handle */
	ISC_STATUS status[20];             /* status vector */
	XSQLDA *out_sqlda;                 /* out sqlda */
	XSQLDA *in_sqlda;                  /* in sqlda */
	Tcl_Obj *database;
	struct dbi_Interbase_Data *parent;
	struct dbi_Interbase_Data **clones;
	int clonesnum;
	char *dpb;
	int dpbpos;
	int dpblen;
	int tuple;
	int ntuples;
	int autocommit;
	int cursor_open;
	int out_sqlda_filled;
} dbi_Interbase_Data;


EXTERN int dbi_Interbase_Init(Tcl_Interp *interp);
