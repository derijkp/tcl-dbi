#define DONT_TD_VOID
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

/*
 * Windows needs to know which symbols to export.  Unix does not.
 * BUILD_Class should be undefined for Unix.
 */

#ifdef BUILD_Extral
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif /* BUILD_Extral */

typedef struct ResultBuffer {
	SDWORD cbValue;
	SDWORD cbValueMax;
	SDWORD fSqlType;
	char*  strResult;
} ResultBuffer;

typedef struct dbi_odbc_Data {
	Tcl_Command token;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	RETCODE rc;
	int hasconn;
	Tcl_Obj *namespace;
	Tcl_Obj *dbms_name;
	Tcl_Obj *dbms_ver;
	int supportpos;
	Tcl_Obj *defnullvalue;
	ResultBuffer *resultbuffer;
	int autocommit;
	int trans;
	int nfields;
	int ntuples;
	int tuple;
} dbi_odbc_Data;

#define DB_OPENCONN(dbdata) (dbdata->hasconn == 1)

void dbi_odbc_error(Tcl_Interp *interp,dbi_odbc_Data *dbdata,long erg,SQLHDBC hdbc,SQLHSTMT hstmt);

EXTERN int dbi_odbc_Init(Tcl_Interp *interp);
