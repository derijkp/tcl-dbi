/* $Format: "#define DBI_VERSION \"0.$ProjectMajorVersion: 8 $\""$ */
#define DBI_VERSION "0.8"

#include <ibase.h>
#include "tcl.h"
/*#include "dbi.h"*/

#define SEGM_SIZE 8192

typedef struct dbi_Firebird_Data {
	Tcl_Command token;
	Tcl_Interp *interp;
	isc_db_handle db;                  /* database handle */
	isc_stmt_handle stmt;              /* statement handle */
	isc_tr_handle trans;               /* transaction handle */
	ISC_STATUS status[20];             /* status vector */
	XSQLDA *out_sqlda;                 /* out sqlda */
	XSQLDA *in_sqlda;                  /* in sqlda */
	Tcl_Obj *database;
	struct dbi_Firebird_Data *parent;
	struct dbi_Firebird_Data **clones;
	int clonesnum;
	Tcl_Obj *defnullvalue;
	char *dpb;
	int dpbpos;
	int dpblen;
	int tuple;
	int ntuples;
	int autocommit;
	int cursor_open;
	Tcl_HashTable preparedhash;
	XSQLDA *out_sqlda_cache;
	XSQLDA *in_sqlda_cache;
	int blobid;
	ISC_QUAD newblob_id;
	isc_blob_handle blob_handle;
	isc_blob_handle newblob_handle;
	char newblob_buffer[SEGM_SIZE];
	int newblob_buffer_free;
} dbi_Firebird_Data;

typedef struct dbi_Firebird_prepared {
	XSQLDA *out_sqlda;
	XSQLDA *in_sqlda;
} dbi_Firebird_Prepared;

EXTERN int dbi_Firebird_Init(Tcl_Interp *interp);

#define EXEC_USEFETCH 1
#define EXEC_FLAT 2
#define EXEC_BLOBID 4
#define EXEC_CACHE 4
