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
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	int hasconn;
	Tcl_Obj *defnullvalue;
	ResultBuffer *resultbuffer;
	int nfields;
	int t;
	int tuples;
} dbi_odbc_Data;

void dbi_odbc_error(Tcl_Interp *interp,long erg,SQLHDBC hdbc,SQLHSTMT hstmt);

EXTERN int dbi_odbc_Init(Tcl_Interp *interp);
