#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

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
