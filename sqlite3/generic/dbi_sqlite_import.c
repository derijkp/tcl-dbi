/*
** following code slightly adapted from tclsqlite
** original license
**
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
*/

#define _FILE_OFFSET_BITS 64

#include "tclInt.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include "dbi_sqlite.h"

/*
** Compute a string length that is limited to what can be stored in
** lower 30 bits of a 32-bit signed integer.
*/
static int strlen30(const char *z){
  const char *z2 = z;
  while( *z2 ){ z2++; }
  return 0x3fffffff & (int)(z2 - z);
}

/*
** This routine reads a line of text from FILE in, stores
** the text in memory obtained from malloc() and returns a pointer
** to the text.  NULL is returned at end of file, or if malloc()
** fails.
**
** The interface is like "readline" but no command-line editing
** is done.
**
** copied from shell.c from '.import' command
*/
static char *local_getline(char *zPrompt, FILE *in){
  char *zLine;
  int nLine;
  int n;
  int eol;

  nLine = 100;
  zLine = malloc( nLine );
  if( zLine==0 ) return 0;
  n = 0;
  eol = 0;
  while( !eol ){
    if( n+100>nLine ){
      nLine = nLine*2 + 100;
      zLine = realloc(zLine, nLine);
      if( zLine==0 ) return 0;
    }
    if( fgets(&zLine[n], nLine - n, in)==0 ){
      if( n==0 ){
        free(zLine);
        return 0;
      }
      zLine[n] = 0;
      eol = 1;
      break;
    }
    while( zLine[n] ){ n++; }
    if( n>0 && zLine[n-1]=='\n' ){
      n--;
      zLine[n] = 0;
      eol = 1;
    }
  }
  zLine = realloc( zLine, n+1 );
  return zLine;
}

int dbi_Sqlite3_Import(
	Tcl_Interp *interp,
	dbi_Sqlite3_Data *dbdata,
	int objc,
	Tcl_Obj *objv[])
{
    char *zTable;               /* Insert data into this table */
    char *zFile;                /* The file from which to extract data */
    char *zConflict;            /* The conflict algorithm to use */
    sqlite3_stmt *pStmt;        /* A statement */
    int nCol;                   /* Number of columns in the table */
    int aCol;                   /* actual number of cols in line */
    int nByte;                  /* Number of bytes in an SQL string */
    int i, j;                   /* Loop counters */
    int nSep;                   /* Number of bytes in zSep[] */
    int nNull;                  /* Number of bytes in zNull[] */
    char *zSql;                 /* An SQL statement */
    char *zLine;                /* A single line of input from the file */
    char **azCol;               /* zLine[] broken up into columns */
    char *zCommit;              /* How to commit changes */
    FILE *in;                   /* The input file */
    int lineno = 0;             /* Line number of input file */
    char zLineNum[80];          /* Line number print buffer */
    Tcl_Obj *pResult;           /* interp result */
    int rc = TCL_OK, size;
    char *zSep;
    char *zNull;
    char *zHeader;
    if( objc<5 || objc>8 ){
      Tcl_WrongNumArgs(interp, 2, objv, 
         "conflict-algorithm table filename ?separator? ?nullindicator? ?header?");
      return TCL_ERROR;
    }
    if( objc>=6 ){
      zSep = Tcl_GetStringFromObj(objv[5], 0);
    }else{
      zSep = "\t";
    }
    if( objc>=7 ){
      zNull = Tcl_GetStringFromObj(objv[6], 0);
    }else{
      zNull = "";
    }
    if( objc>=8 ){
      zHeader = Tcl_GetStringFromObj(objv[7], 0);
    }else{
      zHeader = NULL;
    }
    zConflict = Tcl_GetStringFromObj(objv[2], 0);
    zTable = Tcl_GetStringFromObj(objv[3], 0);
    zFile = Tcl_GetStringFromObj(objv[4], 0);
    nSep = strlen30(zSep);
    nNull = strlen30(zNull);
    if( nSep==0 ){
      Tcl_AppendResult(interp,"Error: non-null separator required for copy",0);
      return TCL_ERROR;
    }
    if(strcmp(zConflict, "rollback") != 0 &&
       strcmp(zConflict, "abort"   ) != 0 &&
       strcmp(zConflict, "fail"    ) != 0 &&
       strcmp(zConflict, "ignore"  ) != 0 &&
       strcmp(zConflict, "replace" ) != 0 ) {
      Tcl_AppendResult(interp, "Error: \"", zConflict, 
            "\", conflict-algorithm must be one of: rollback, "
            "abort, fail, ignore, or replace", 0);
      return TCL_ERROR;
    }
    in = fopen(zFile, "rb");
    if( in==0 ){
      Tcl_AppendResult(interp, "Error: cannot open file: ", zFile, NULL);
      return TCL_ERROR;
    }
    zSql = sqlite3_mprintf("SELECT * FROM '%q'", zTable);
    if( zSql==0 ){
      Tcl_AppendResult(interp, "Error: no such table: ", zTable, 0);
      return TCL_ERROR;
    }
    nByte = strlen30(zSql);
    if (zHeader == NULL) {
	    rc = sqlite3_prepare(dbdata->db, zSql, -1, &pStmt, 0);
	    sqlite3_free(zSql);
	    if( rc ){
	      Tcl_AppendResult(interp, "Error: ", sqlite3_errmsg(dbdata->db), 0);
	      nCol = 0;
	    }else{
	      nCol = sqlite3_column_count(pStmt);
	    }
	    sqlite3_finalize(pStmt);
    } else {
	Tcl_Obj **elemPtrs;
	Tcl_Obj *resObjPtr,*listPtr=NULL;
	char *bytes;
	int length,error;
	sqlite3_free(zSql);
	error = Tcl_ListObjGetElements(interp, objv[7], &nCol, &elemPtrs);
	if (error) {return error;}
	if (nCol == 0) {
		Tcl_Obj *objPtr;
		char *string,*p,*end;
		listPtr = Tcl_NewListObj(0,NULL);
		zHeader = local_getline(0, in);
		string = zHeader;
		while (*string && (p = strchr(string, (int) *zSep)) != NULL) {
			objPtr = Tcl_NewStringObj(string, p - string);
			error = Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
			if (error) {Tcl_DecrRefCount(listPtr); Tcl_DecrRefCount(objPtr); return error;}
			string = p + 1;
		}
		end = string + strlen(string);
		objPtr = Tcl_NewStringObj(string, end - string);
		Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
		error = Tcl_ListObjGetElements(interp, listPtr, &nCol, &elemPtrs);
		if (error) {Tcl_DecrRefCount(listPtr); return error;}
	}
	resObjPtr = Tcl_NewListObj(0,NULL);
	Tcl_AppendToObj(resObjPtr, "\"", 1);
	for (i = 0;  i < nCol;  i++) {
		bytes = Tcl_GetStringFromObj(elemPtrs[i], &length);
		if (i > 0) {
			Tcl_AppendToObj(resObjPtr, "\",\"", 3);
		}
		Tcl_AppendToObj(resObjPtr, bytes, length);
	}
	Tcl_AppendToObj(resObjPtr, "\"", 1);
	bytes = Tcl_GetStringFromObj(resObjPtr,&length);
	zHeader = Tcl_Alloc((length+1)*sizeof(char));
	strncpy(zHeader,bytes,length);
	zHeader[length] = '\0';
	Tcl_DecrRefCount(resObjPtr);
	if (listPtr != NULL) {Tcl_DecrRefCount(listPtr);}
    }
    if( nCol==0 ) {
      return TCL_ERROR;
    }
    if (zHeader != NULL) {
	size = nByte + 52 + strlen(zHeader);
    } else {
	size = nByte + 50;
    }
    zSql = malloc( size + nCol*2 );
    if( zSql==0 ) {
      Tcl_AppendResult(interp, "Error: can't malloc()", 0);
      return TCL_ERROR;
    }
    if (zHeader != NULL) {
        sqlite3_snprintf(size, zSql, "INSERT OR %q INTO '%q'(%s) VALUES(?",zConflict, zTable, zHeader);
    } else {
        sqlite3_snprintf(size, zSql, "INSERT OR %q INTO '%q' VALUES(?",zConflict, zTable);
    }
    j = strlen30(zSql);
    for(i=1; i<nCol; i++){
      zSql[j++] = ',';
      zSql[j++] = '?';
    }
    zSql[j++] = ')';
    zSql[j] = 0;
    rc = sqlite3_prepare(dbdata->db, zSql, -1, &pStmt, 0);
    free(zSql);
    if( rc ){
      Tcl_AppendResult(interp, "Error: ", sqlite3_errmsg(dbdata->db), 0);
      sqlite3_finalize(pStmt);
      return TCL_ERROR;
    }
    azCol = malloc( sizeof(azCol[0])*(nCol+1) );
    if( azCol==0 ) {
      Tcl_AppendResult(interp, "Error: can't malloc()", 0);
      fclose(in);
      return TCL_ERROR;
    }
    (void)sqlite3_exec(dbdata->db, "BEGIN", 0, 0, 0);
    zCommit = "COMMIT";
    while( (zLine = local_getline(0, in))!=0 ){
      char *z;
      i = 0;
      lineno++;
      azCol[0] = zLine;
      for(i=0, z=zLine; *z; z++){
        if( *z==zSep[0] && strncmp(z, zSep, nSep)==0 ){
          *z = 0;
          i++;
          if( i<nCol ){
            azCol[i] = &z[nSep];
            z += nSep-1;
          }
        }
      }
      aCol = i+1;
      if( i+1>nCol ){
        char *zErr;
        int nErr = strlen30(zFile) + 200;
        zErr = malloc(nErr);
        if( zErr ){
          sqlite3_snprintf(nErr, zErr,
             "Error: %s line %d: expected %d columns of data but found %d",
             zFile, lineno, nCol, i+1);
          Tcl_AppendResult(interp, zErr, 0);
          free(zErr);
        }
        zCommit = "ROLLBACK";
        break;
      }
      for(i=0; i<nCol; i++){
        /* check for null data, if so, bind as null */
        if( i >= aCol || (nNull>0 && strcmp(azCol[i], zNull)==0)
          || strlen30(azCol[i])==0 
        ){
          sqlite3_bind_null(pStmt, i+1);
        }else{
          sqlite3_bind_text(pStmt, i+1, azCol[i], -1, SQLITE_STATIC);
        }
      }
      sqlite3_step(pStmt);
      rc = sqlite3_reset(pStmt);
      free(zLine);
      if( rc!=SQLITE_OK ){
        Tcl_AppendResult(interp,"Error: ", sqlite3_errmsg(dbdata->db), 0);
        zCommit = "ROLLBACK";
        break;
      }
    }
    free(azCol);
    fclose(in);
    sqlite3_finalize(pStmt);
    (void)sqlite3_exec(dbdata->db, zCommit, 0, 0, 0);

    if( zCommit[0] == 'C' ){
      /* success, set result as number of lines processed */
      pResult = Tcl_GetObjResult(interp);
      Tcl_SetIntObj(pResult, lineno);
      rc = TCL_OK;
    }else{
      /* failure, append lineno where failed */
      sqlite3_snprintf(sizeof(zLineNum), zLineNum,"%d",lineno);
      Tcl_AppendResult(interp,", failed while processing line: ",zLineNum,0);
      rc = TCL_ERROR;
    }
    return rc;
}
