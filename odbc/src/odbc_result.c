/*
 *       File:    odbc.c
 *       Purpose: dbi extension to Tcl: odbc backend
 *       Author:  Copyright (c) 1998 Peter De Rijk
 *
 *       See the file "README" for information on usage and redistribution
 *       of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <string.h>
#include <stdlib.h>
#include "tcl.h"
#include "dbi.h"
#include "odbc.h"

int dbi_odbc_initresult(
	Tcl_Interp *interp,
	Dbi *db)
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)db->dbdata;
	ResultBuffer *resultbuffer = NULL;
	RETCODE rc;
	UWORD i;
	SWORD tmp;
	int error;
	/* number of fields */
	rc = SQLNumResultCols(dbdata->hstmt, &tmp);
	if (rc == SQL_ERROR) {
		Tcl_AppendResult(interp,"ODBC Error getting number of result columns: ",NULL);
		dbi_odbc_error(interp,rc,SQL_NULL_HDBC,dbdata->hstmt);
		return TCL_ERROR;
	}
	dbdata->nfields = (int)tmp;
	/* init */
	dbdata->t = -1;
	if (tmp == 0) {return TCL_OK;}
	resultbuffer = (ResultBuffer*) Tcl_Alloc(dbdata->nfields*sizeof(ResultBuffer));
	if (!resultbuffer) {Tcl_AppendResult(interp,"Memory allocation error");return TCL_ERROR;}
	memset (resultbuffer, 0, dbdata->nfields*sizeof(resultbuffer));
	for (i=0; i<dbdata->nfields; ++i) {
		/* max column length */
		rc = SQLColAttributes(dbdata->hstmt, (UWORD)(i+1), SQL_COLUMN_DISPLAY_SIZE, 
			NULL, 0, NULL, &resultbuffer[i].cbValueMax);
		if (rc == SQL_ERROR) {
			Tcl_AppendResult(interp,"ODBC Error getting column length: ",NULL);
			dbi_odbc_error(interp,rc,SQL_NULL_HDBC,dbdata->hstmt);
			goto error;
		}
		/* column type */
		rc = SQLColAttributes(dbdata->hstmt, (UWORD)(i+1), SQL_COLUMN_TYPE, 
			NULL, 0, NULL, &resultbuffer[i].fSqlType);
		if (rc == SQL_ERROR) {
			Tcl_AppendResult(interp,"ODBC Error getting column type: ",NULL);
			dbi_odbc_error(interp,rc,SQL_NULL_HDBC,dbdata->hstmt);
			goto error;
		}
		/* determine amount for space to allocate */
		if (resultbuffer[i].cbValueMax == SQL_NO_TOTAL ||
			resultbuffer[i].fSqlType == SQL_LONGVARBINARY || 
			resultbuffer[i].fSqlType == SQL_LONGVARCHAR) {
			/* column not bound */
			resultbuffer[i].strResult = NULL;
			resultbuffer[i].cbValueMax = 0;
		} else {
			resultbuffer[i].strResult = (char*)Tcl_Alloc((resultbuffer[i].cbValueMax+1)*sizeof(char));
			if (!resultbuffer[i].strResult) {Tcl_AppendResult(interp,"Memory allocation error"); goto error;}
			memset (resultbuffer[i].strResult, 0, resultbuffer[i].cbValueMax*sizeof(char)+1);
			/* bind */
			rc = SQLBindCol(dbdata->hstmt, (UWORD)(i+1), SQL_C_CHAR, resultbuffer[i].strResult, 
				resultbuffer[i].cbValueMax+1, &(resultbuffer[i].cbValue));
			if (rc == SQL_ERROR) {
				Tcl_AppendResult(interp,"ODBC Error binding column: ",NULL);
				dbi_odbc_error(interp,rc,SQL_NULL_HDBC,dbdata->hstmt);
				goto error;
			}
		}
	}
	dbdata->resultbuffer = resultbuffer;
	return TCL_OK;
	error:
		dbi_odbc_clearresult(interp,db);
		return TCL_ERROR;
}

int dbi_odbc_clearresult(
	Tcl_Interp *interp,
	Dbi *db)
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)db->dbdata;
	int i;
	ResultBuffer *resultbuffer = dbdata->resultbuffer;
	SQLFreeHandle(SQL_HANDLE_STMT,dbdata->hstmt);
	if (resultbuffer == NULL) {return TCL_OK;}
	for (i=0; i<dbdata->nfields; ++i) {
		if (resultbuffer[i].strResult) {Tcl_Free(resultbuffer[i].strResult);}
	}
	Tcl_Free((char*)resultbuffer);
	dbdata->resultbuffer = NULL;
	dbdata->nfields = -1;
	return TCL_OK;
}

#ifdef NEVER
int dbi_odbc_GetOne(
	Tcl_Interp *interp,
	Dbi *db,
	int i,
	Tcl_Obj **resultPtr)
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)db->dbdata;
	Tcl_Obj *line = NULL, *element = NULL;
	RETCODE rc;
	char *string, *buffer = NULL;
	int error;
	ResultBuffer *resultbuffer = dbdata->resultbuffer;
	if (!resultbuffer[i].strResult) {
		/* get length of variable fields by setting bufsize = 1 */
		rc = SQLGetData(dbdata->hstmt, i+1, SQL_C_CHAR, "", 1, &(resultbuffer[i].cbValue)); 
		if (rc == SQL_ERROR) {
			Tcl_AppendResult(interp,"ODBC Error SQLGetData: ",NULL);
			dbi_odbc_error(interp,rc,SQL_NULL_HDBC,dbdata->hstmt);
			goto error;
		}
	}
	if (resultbuffer[i].cbValue == SQL_NULL_DATA) {
		element = NULL;
	} else {
		if (resultbuffer[i].strResult) {
			/* append value from bound buffer element */
			if (resultbuffer[i].cbValue < resultbuffer[i].cbValueMax) {
				element = Tcl_NewStringObj(resultbuffer[i].strResult, resultbuffer[i].cbValue);
			} else {
				element = Tcl_NewStringObj(resultbuffer[i].strResult, resultbuffer[i].cbValueMax);
			}
		} else {
			buffer = (char *)Tcl_Realloc(buffer,resultbuffer[i].cbValue);
			rc = SQLGetData(dbdata->hstmt, i+1, SQL_C_CHAR, (char*) buffer, 
		 	   resultbuffer[i].cbValue + 1, &(resultbuffer[i].cbValue)); 
			if (rc == SQL_ERROR) {
				Tcl_AppendResult(interp,"ODBC Error SQLGetData: ",NULL);
				dbi_odbc_error(interp,rc,SQL_NULL_HDBC,dbdata->hstmt);
				goto error;
			}
			element = Tcl_NewStringObj(buffer,resultbuffer[i].cbValue);
		}
	}
	if (buffer != NULL) {Tcl_Free(buffer);}
	*resultPtr = element;
	return TCL_OK;;
	error:
		if (buffer != NULL) {Tcl_Free(buffer);}
		if (element != NULL) Tcl_DecrRefCount(element);
		return TCL_ERROR;
}
#endif

int dbi_odbc_GetOne(
	Tcl_Interp *interp,
	Dbi *db,
	int i,
	Tcl_Obj **resultPtr)
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)db->dbdata;
	ResultBuffer *resultbuffer = dbdata->resultbuffer;
	Tcl_Obj *line = NULL, *element = NULL;
	RETCODE rc;
	char *string, *buffer = NULL;
	int error;
	if (!resultbuffer[i].strResult) {
		buffer = (char *)Tcl_Alloc(2*sizeof(char));
		/* get length of variable fields by setting bufsize = 1 */
		rc = SQLGetData(dbdata->hstmt, i+1, SQL_C_CHAR, buffer, 1, &(resultbuffer[i].cbValue)); 
		if (rc == SQL_ERROR) {
			Tcl_AppendResult(interp,"ODBC Error SQLGetData: ",NULL);
			dbi_odbc_error(interp,rc,SQL_NULL_HDBC,dbdata->hstmt);
			goto error;
		}
	}
	if (resultbuffer[i].cbValue == SQL_NULL_DATA) {
		element = NULL;
	} else {
		if (resultbuffer[i].strResult) {
			/* append value from bound buffer element */
			if (resultbuffer[i].cbValue < resultbuffer[i].cbValueMax) {
				element = Tcl_NewStringObj(resultbuffer[i].strResult, resultbuffer[i].cbValue);
			} else {
				element = Tcl_NewStringObj(resultbuffer[i].strResult, resultbuffer[i].cbValueMax);
			}
		} else {
			buffer = (char *)Tcl_Realloc(buffer,(resultbuffer[i].cbValue+1)*sizeof(char));
			rc = SQLGetData(dbdata->hstmt, i+1, SQL_C_CHAR, (char*) buffer, 
		 	   resultbuffer[i].cbValue + 1, &(resultbuffer[i].cbValue)); 
			if (rc == SQL_ERROR) {
				Tcl_AppendResult(interp,"ODBC Error SQLGetData: ",NULL);
				dbi_odbc_error(interp,rc,SQL_NULL_HDBC,dbdata->hstmt);
				goto error;
			}
			element = Tcl_NewStringObj(buffer,resultbuffer[i].cbValue);
		}
	}
	if (buffer != NULL) {Tcl_Free(buffer);}
	*resultPtr = element;
	return TCL_OK;;
	error:
		if (buffer != NULL) {Tcl_Free(buffer);}
		if (element != NULL) Tcl_DecrRefCount(element);
		return TCL_ERROR;
}

int dbi_odbc_GetRow(
	Tcl_Interp *interp,
	Dbi *db,
	Tcl_Obj *nullvalue,
	Tcl_Obj **resultPtr)
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)db->dbdata;
	ResultBuffer *resultbuffer = dbdata->resultbuffer;
	Tcl_Obj *line = NULL, *element = NULL;
	char *string;
	int i,error;
	line = Tcl_NewListObj(0,NULL);
	for (i=0; i<dbdata->nfields; ++i) {
		error = dbi_odbc_GetOne(interp,db,i,&element);
		if (element == NULL) {
	 	   element = nullvalue;
		}
		error = Tcl_ListObjAppendElement(interp,line,element);
		if (error) goto error;
	}
	*resultPtr = line;
	return TCL_OK;;
	error:
		if (line != NULL) Tcl_DecrRefCount(line);
		if (element != NULL) Tcl_DecrRefCount(element);
		return TCL_ERROR;
}

int dbi_odbc_ToResult(
	Tcl_Interp *interp,
	Dbi *db,
	Tcl_Obj *nullvalue)
{
	dbi_odbc_Data *dbdata = (dbi_odbc_Data *)db->dbdata;
	Tcl_Obj *result = NULL, *line = NULL;
	int error;
	RETCODE rc;
	if (nullvalue == NULL) {
		nullvalue = 	dbdata->defnullvalue;
;
	}
	result = Tcl_NewListObj(0,NULL);
	while(1) {
		rc = SQLFetch(dbdata->hstmt);
		if (rc == SQL_NO_DATA_FOUND) {
			break;
		} else if (rc != SQL_SUCCESS) {
			Tcl_AppendResult(interp,"ODBC Error SQLFetch: ",NULL);
			dbi_odbc_error(interp,rc,SQL_NULL_HDBC,dbdata->hstmt);
			goto error;
		}
		error = dbi_odbc_GetRow(interp,db,nullvalue,&line);
		if (error) {goto error;}
		error = Tcl_ListObjAppendElement(interp,result, line);
		if (error) goto error;
		line = NULL;
	}
	Tcl_SetObjResult(interp, result);
	return TCL_OK;;
	error:
		if (result != NULL) Tcl_DecrRefCount(result);
		if (line != NULL) Tcl_DecrRefCount(line);
		return TCL_ERROR;
}