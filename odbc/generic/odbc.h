/* $Format: "#define DBI_VERSION \"$ProjectMajorVersion$.$ProjectMinorVersion$\""$ */
#define DBI_VERSION "1.0"

#define DONT_TD_VOID
#ifdef __WIN32__
#include <windows.h>
#endif
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

typedef struct odbc_Result {
	ResultBuffer *buffer;
	int nfields;
	int ntuples;
	int tuple;
} odbc_Result;

typedef struct ParamBuffer {
	char*  paramdata;
	SQLINTEGER arglen;
} ParamBuffer;

typedef struct dbi_odbc_Data {
	Tcl_Command token;
	Tcl_Interp *interp;
	SQLHDBC hdbc;
	SQLHSTMT hstmt;
	RETCODE rc;
	int hasconn;
	Tcl_Obj *dbms_name;
	Tcl_Obj *dbms_ver;
	int supportpos;
	Tcl_Obj *defnullvalue;
	odbc_Result result;
	int autocommit;
	int trans;
	struct dbi_odbc_Data *parent;
	Tcl_Command *clones;
	int clonesnum;
} dbi_odbc_Data;

#define DB_OPENCONN(dbdata) (dbdata->hasconn == 1)

#define CATALOG_SQLColumns 1
#define CATALOG_SQLPrimaryKeys 2
#define CATALOG_SQLColumnPrivileges 3
#define CATALOG_SQLForeignKeys 4
#define CATALOG_SQLSpecialColumns 5
#define CATALOG_SQLStatistics 6
#define CATALOG_SQLTablePrivileges 7
#define CATALOG_SQLTables 8
#define CATALOG_SQLProcedures 9
#define CATALOG_SQLProcedureColumns 10



void dbi_odbc_error(Tcl_Interp *interp,long erg,SQLHDBC hdbc,SQLHSTMT hstmt);
int dbi_odbc_initresult(odbc_Result *result);
int dbi_odbc_bindresult(Tcl_Interp *interp,SQLHSTMT hstmt, odbc_Result *result);
int dbi_odbc_clearresult(SQLHSTMT hstmt, odbc_Result *result);
int dbi_odbc_GetOne(Tcl_Interp *interp,SQLHSTMT hstmt, odbc_Result *result,int i,Tcl_Obj **resultPtr);
int dbi_odbc_GetRow(Tcl_Interp *interp,SQLHSTMT hstmt, odbc_Result *result,Tcl_Obj *nullvalue,Tcl_Obj **resultPtr);
int dbi_odbc_ToResult(Tcl_Interp *interp,SQLHSTMT hstmt, odbc_Result *result,Tcl_Obj *nullvalue);

EXTERN int dbi_odbc_Init(Tcl_Interp *interp);
