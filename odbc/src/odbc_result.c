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
#include "odbc.h"

int dbi_odbc_initresult(
	odbc_Result *dbresult)
{
	dbresult->buffer = NULL;
	dbresult->nfields = 0;
	dbresult->ntuples = -1;
	dbresult->tuple = -1;
	return TCL_OK;
}

int dbi_odbc_clearresult(
	SQLHSTMT hstmt,
	odbc_Result *dbresult)
{
	int i;
/*fprintf(stdout,"--> clear ? %d\n",*nfields);fflush(stdout);*/
	if (dbresult->nfields != 0) {
		SQLCloseCursor(hstmt);
	}
	if (dbresult->buffer != NULL) {
		/*fprintf(stdout,"clear result\n");fflush(stdout);*/
		for (i=0; i< dbresult->nfields; ++i) {
			if (dbresult->buffer[i].strResult) {Tcl_Free(dbresult->buffer[i].strResult);}
		}
		Tcl_Free((char*)dbresult->buffer);
		dbresult->buffer = NULL;
	}
	dbresult->nfields = 0;
	dbresult->ntuples = -1;
	dbresult->tuple = -1;
	return TCL_OK;
}

int dbi_odbc_bindresult(
	Tcl_Interp *interp,
	HSTMT hstmt,
	odbc_Result *dbresult)
{
	RETCODE rc;
	UWORD i;
	SWORD tmp;
	SQLINTEGER tmp2;
	ResultBuffer *resultbuffer;
	int nfields,ntuples;
	if (dbresult->buffer != NULL) {
		Tcl_AppendResult(interp,"resultbuffer not empty",NULL);
		return TCL_ERROR;
	}
	/* number of fields */
	rc = SQLNumResultCols(hstmt, &tmp);
	if (rc == SQL_ERROR) {
		Tcl_AppendResult(interp,"ODBC Error getting number of result columns: ",NULL);
		dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
		return TCL_ERROR;
	}
	nfields = (int)tmp;
	dbresult->nfields = nfields;
	if (nfields <= 0) {
		dbresult->buffer = NULL;
		dbresult->ntuples = 0;
		return TCL_OK;
	}
	rc = SQLRowCount(hstmt, &tmp2);
	if (rc == SQL_ERROR) {
		Tcl_AppendResult(interp,"ODBC Error getting number of result rows: ",NULL);
		dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
		dbresult->buffer = NULL;
		return TCL_ERROR;
	}
	ntuples = (int)tmp2;
	dbresult->ntuples = ntuples;
	dbresult->tuple = -1;
	/* init */
	if (tmp == 0) {
		dbresult->buffer = NULL;
		return TCL_OK;
	}
	SQLFreeStmt(hstmt,SQL_UNBIND);
	resultbuffer = (ResultBuffer*) Tcl_Alloc(nfields*sizeof(ResultBuffer));
	if (!resultbuffer) {
		dbresult->buffer = NULL;
		Tcl_AppendResult(interp,"Memory allocation error");
		return TCL_ERROR;
	}
	memset (resultbuffer, 0, nfields*sizeof(resultbuffer));
	for (i=0; i<nfields; ++i) {
		/* max column length */
		rc = SQLColAttributes(hstmt, (UWORD)(i+1), SQL_COLUMN_DISPLAY_SIZE, 
			NULL, 0, NULL, &resultbuffer[i].cbValueMax);
		if (rc == SQL_ERROR) {
			Tcl_AppendResult(interp,"ODBC Error getting column length: ",NULL);
			dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
			goto error;
		}
		/* column type */
		rc = SQLColAttributes(hstmt, (UWORD)(i+1), SQL_COLUMN_TYPE, 
			NULL, 0, NULL, &resultbuffer[i].fSqlType);
		if (rc == SQL_ERROR) {
			Tcl_AppendResult(interp,"ODBC Error getting column type: ",NULL);
			dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
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
			int size;
			SQLSMALLINT type;
			switch(resultbuffer[i].fSqlType) {
				case SQL_DOUBLE:
				case SQL_FLOAT:
					size = sizeof(double);
					type = SQL_C_DOUBLE;
					break;
				case SQL_TYPE_DATE:
					size = sizeof(DATE_STRUCT);
					type = SQL_C_TYPE_DATE;
					break;
				case SQL_TYPE_TIME:
					size = sizeof(TIME_STRUCT);
					type = SQL_C_TYPE_TIME;
					break;
				case SQL_TYPE_TIMESTAMP:
					size = sizeof(TIMESTAMP_STRUCT);
					type = SQL_C_TYPE_TIMESTAMP;
					break;
				default:
					size = (resultbuffer[i].cbValueMax+1)*sizeof(char);
					type = SQL_C_CHAR;
					break;
			}
			resultbuffer[i].strResult = (char*)Tcl_Alloc(size);
			if (!resultbuffer[i].strResult) {Tcl_AppendResult(interp,"Memory allocation error"); goto error;}
			memset (resultbuffer[i].strResult, 0, size);
			/* bind */
			rc = SQLBindCol(hstmt, (UWORD)(i+1), type, resultbuffer[i].strResult, 
				resultbuffer[i].cbValueMax+1, &(resultbuffer[i].cbValue));
			if (rc == SQL_ERROR) {
				Tcl_AppendResult(interp,"ODBC Error binding column: ",NULL);
				dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
				goto error;
			}
		}
	}
	dbresult->buffer = resultbuffer;
	return TCL_OK;
	error:
		dbi_odbc_clearresult(hstmt,dbresult);
		return TCL_ERROR;
}

int dbi_odbc_GetOne(
	Tcl_Interp *interp,
	HSTMT hstmt,
	odbc_Result *dbresult,
	int i,
	Tcl_Obj **resultPtr)
{
	ResultBuffer *resultbuffer = dbresult->buffer;
	Tcl_Obj *element = NULL;
	RETCODE rc;
	SDWORD size;
	char sbuffer[25];
	char *buffer = NULL, *usebuffer = NULL;
	if (resultbuffer[i].cbValue == SQL_NULL_DATA) {
		*resultPtr = NULL;
		return TCL_OK;
	} else {
		if (resultbuffer[i].strResult) {
			switch(resultbuffer[i].fSqlType) {
				case SQL_DOUBLE:
				case SQL_FLOAT:
					{
					double *dtemp = (double *)(resultbuffer[i].strResult);
					*resultPtr = Tcl_NewDoubleObj((double)*dtemp);
					return TCL_OK;
					}
				case SQL_TYPE_DATE:
					{
					DATE_STRUCT *temp = (DATE_STRUCT *)(resultbuffer[i].strResult);
					sprintf(sbuffer,"%4d-%2d-%2d",temp->year,temp->month,temp->day);
					*resultPtr = Tcl_NewStringObj(sbuffer,10);
					return TCL_OK;
					}
				case SQL_TYPE_TIME:
					{
					TIME_STRUCT *temp = (TIME_STRUCT *)(resultbuffer[i].strResult);
					sprintf(sbuffer,"%2d:%2d:%2d.000",temp->hour,temp->minute,temp->second);
					*resultPtr = Tcl_NewStringObj(sbuffer,12);
					return TCL_OK;
					}
				case SQL_TYPE_TIMESTAMP:
					{
					TIMESTAMP_STRUCT *temp = (TIMESTAMP_STRUCT *)(resultbuffer[i].strResult);
					sprintf(sbuffer,"%4d-%2d-%2d %2d:%2d:%2d.%03d",
						(int)temp->year,(int)temp->month,(int)temp->day,(int)temp->hour,(int)temp->minute,(int)temp->second,(int)(temp->fraction/1000000));
					*resultPtr = Tcl_NewStringObj(sbuffer,23);
					return TCL_OK;
					}
			}
			/* append value from bound buffer element */
			usebuffer = resultbuffer[i].strResult;
			if (resultbuffer[i].cbValue < resultbuffer[i].cbValueMax) {
				size = resultbuffer[i].cbValue;
			} else {
				size = resultbuffer[i].cbValueMax;
			}
		} else {
			buffer = (char *)Tcl_Alloc(2*sizeof(char));
			/* get length of variable fields by setting bufsize = 1 */
			rc = SQLGetData(hstmt, i+1, SQL_C_CHAR, buffer, 1, &(resultbuffer[i].cbValue)); 
			if (rc == SQL_ERROR) {
				Tcl_AppendResult(interp,"ODBC Error SQLGetData: ",NULL);
				dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
				goto error;
			}
			/* allocate the proper length, and get data */
			buffer = (char *)Tcl_Realloc(buffer,(resultbuffer[i].cbValue+1)*sizeof(char));
			usebuffer = buffer;
			size = resultbuffer[i].cbValue;
			rc = SQLGetData(hstmt, i+1, SQL_C_CHAR, (char*) buffer, 
		 	   resultbuffer[i].cbValue + 1, &(resultbuffer[i].cbValue)); 
			if (rc == SQL_ERROR) {
				Tcl_AppendResult(interp,"ODBC Error SQLGetData: ",NULL);
				dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
				goto error;
			}
		}
		if (resultbuffer[i].fSqlType == SQL_CHAR) {
			while ((size > 0) && (usebuffer[(int)size-1] == ' ')) {
				size--;
			}
		}
		element = Tcl_NewStringObj(usebuffer,size);
	}
	if (buffer != NULL) {Tcl_Free(buffer);}
	*resultPtr = element;
	return TCL_OK;
	error:
		if (buffer != NULL) {Tcl_Free(buffer);}
		if (element != NULL) Tcl_DecrRefCount(element);
		return TCL_ERROR;
}

int dbi_odbc_GetRow(
	Tcl_Interp *interp,
	HSTMT hstmt,
	odbc_Result *dbresult,
	Tcl_Obj *nullvalue,
	Tcl_Obj **resultPtr)
{
	ResultBuffer *resultbuffer = dbresult->buffer;
	Tcl_Obj *line = NULL, *element = NULL;
	int i,error;
	if (resultbuffer == NULL) {
		Tcl_AppendResult(interp, "no result available: invoke exec method with -usefetch option first", NULL);
		return TCL_ERROR;
	}
	line = Tcl_NewListObj(0,NULL);
	for (i=0; i<dbresult->nfields; ++i) {
		error = dbi_odbc_GetOne(interp,hstmt,dbresult,i,&element);
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
	HSTMT hstmt,
	odbc_Result *dbresult,
	Tcl_Obj *nullvalue)
{
	Tcl_Obj *result = NULL, *line = NULL;
	int error;
	RETCODE rc;
	if (dbresult->nfields == 0) {return TCL_OK;}
	result = Tcl_NewListObj(0,NULL);
	while(1) {
		rc = SQLFetch(hstmt);
		if (rc == SQL_NO_DATA_FOUND) {
			break;
		} else if (rc != SQL_SUCCESS) {
			Tcl_AppendResult(interp,"ODBC Error SQLFetch: ",NULL);
			dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
			goto error;
		}
		error = dbi_odbc_GetRow(interp,hstmt,dbresult,nullvalue,&line);
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

int dbi_odbc_ToResult_flat(
	Tcl_Interp *interp,
	HSTMT hstmt,
	odbc_Result *dbresult,
	Tcl_Obj *nullvalue)
{
	Tcl_Obj *result = NULL, *element = NULL;
	int error,i;
	RETCODE rc;
	if (dbresult->nfields == 0) {return TCL_OK;}
	result = Tcl_NewListObj(0,NULL);
	while(1) {
		rc = SQLFetch(hstmt);
		if (rc == SQL_NO_DATA_FOUND) {
			break;
		} else if (rc != SQL_SUCCESS) {
			Tcl_AppendResult(interp,"ODBC Error SQLFetch: ",NULL);
			dbi_odbc_error(interp,rc,SQL_NULL_HDBC,hstmt);
			goto error;
		}
		for (i = 0; i < dbresult->nfields; ++i) {
			error = dbi_odbc_GetOne(interp,hstmt,dbresult,i,&element);
			if (element == NULL) {
		 	   element = nullvalue;
			}
			error = Tcl_ListObjAppendElement(interp,result,element);
			if (error) goto error;
		}
	}
	Tcl_SetObjResult(interp, result);
	return TCL_OK;;
	error:
		if (result != NULL) Tcl_DecrRefCount(result);
		if (element != NULL) Tcl_DecrRefCount(element);
		return TCL_ERROR;
}
