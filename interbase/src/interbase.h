#include <ibase.h>
#include "tcl.h"
#include "dbi.h"

typedef struct dbi_Interbase_Data {
	isc_db_handle db;                  /* database handle */
	isc_stmt_handle stmt;              /* statement handle */
	isc_tr_handle trans;               /* transaction handle */
	ISC_STATUS status[20];             /* status vector */
	XSQLDA *out_sqlda;                 /* out sqlda */
	XSQLDA *in_sqlda;                  /* in sqlda */
	char *dpb;
	int dpblen;
	int autocommit;
	int t;
	int nrows;
	int respos;
} dbi_Interbase_Data;

EXTERN int dbi_Interbase_Init(Tcl_Interp *interp);
