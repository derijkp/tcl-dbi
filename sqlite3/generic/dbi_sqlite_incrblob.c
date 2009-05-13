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
** A TCL Interface to SQLite.  Append this file to sqlite3.c and
** compile the whole thing to build a TCL-enabled version of SQLite.
*/
#include "tclInt.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include "dbi_sqlite.h"

/*
** Close all incrblob channels opened using database connection pDb.
** This is called when shutting down the database connection.
*/
void closeIncrblobChannels(dbi_Sqlite3_Data *pDb){
  IncrblobChannel *p;
  IncrblobChannel *pNext;

  for(p=pDb->pIncrblob; p; p=pNext){
    pNext = p->pNext;

    /* Note: Calling unregister here call Tcl_Close on the incrblob channel, 
    ** which deletes the IncrblobChannel structure at *p. So do not
    ** call Tcl_Free() here.
    */
    Tcl_UnregisterChannel(pDb->interp, p->channel);
  }
}

/*
** Close an incremental blob channel.
*/
static int incrblobClose(ClientData instanceData, Tcl_Interp *interp){
  IncrblobChannel *p = (IncrblobChannel *)instanceData;
  int rc = sqlite3_blob_close(p->pBlob);
  sqlite3 *db = p->pDb->db;

  /* Remove the channel from the dbi_Sqlite3_Data.pIncrblob list. */
  if( p->pNext ){
    p->pNext->pPrev = p->pPrev;
  }
  if( p->pPrev ){
    p->pPrev->pNext = p->pNext;
  }
  if( p->pDb->pIncrblob==p ){
    p->pDb->pIncrblob = p->pNext;
  }

  /* Free the IncrblobChannel structure */
  Tcl_Free((char *)p);

  if( rc!=SQLITE_OK ){
    Tcl_SetResult(interp, (char *)sqlite3_errmsg(db), TCL_VOLATILE);
    return TCL_ERROR;
  }
  return TCL_OK;
}

/*
** Read data from an incremental blob channel.
*/
static int incrblobInput(
  ClientData instanceData, 
  char *buf, 
  int bufSize,
  int *errorCodePtr
){
  IncrblobChannel *p = (IncrblobChannel *)instanceData;
  int nRead = bufSize;         /* Number of bytes to read */
  int nBlob;                   /* Total size of the blob */
  int rc;                      /* sqlite error code */

  nBlob = sqlite3_blob_bytes(p->pBlob);
  if( (p->iSeek+nRead)>nBlob ){
    nRead = nBlob-p->iSeek;
  }
  if( nRead<=0 ){
    return 0;
  }

  rc = sqlite3_blob_read(p->pBlob, (void *)buf, nRead, p->iSeek);
  if( rc!=SQLITE_OK ){
    *errorCodePtr = rc;
    return -1;
  }

  p->iSeek += nRead;
  return nRead;
}

/*
** Write data to an incremental blob channel.
*/
static int incrblobOutput(
  ClientData instanceData, 
  CONST char *buf, 
  int toWrite,
  int *errorCodePtr
){
  IncrblobChannel *p = (IncrblobChannel *)instanceData;
  int nWrite = toWrite;        /* Number of bytes to write */
  int nBlob;                   /* Total size of the blob */
  int rc;                      /* sqlite error code */

  nBlob = sqlite3_blob_bytes(p->pBlob);
  if( (p->iSeek+nWrite)>nBlob ){
    *errorCodePtr = EINVAL;
    return -1;
  }
  if( nWrite<=0 ){
    return 0;
  }

  rc = sqlite3_blob_write(p->pBlob, (void *)buf, nWrite, p->iSeek);
  if( rc!=SQLITE_OK ){
    *errorCodePtr = EIO;
    return -1;
  }

  p->iSeek += nWrite;
  return nWrite;
}

/*
** Seek an incremental blob channel.
*/
static int incrblobSeek(
  ClientData instanceData, 
  long offset,
  int seekMode,
  int *errorCodePtr
){
  IncrblobChannel *p = (IncrblobChannel *)instanceData;

  switch( seekMode ){
    case SEEK_SET:
      p->iSeek = offset;
      break;
    case SEEK_CUR:
      p->iSeek += offset;
      break;
    case SEEK_END:
      p->iSeek = sqlite3_blob_bytes(p->pBlob) + offset;
      break;

    default: assert(!"Bad seekMode");
  }

  return p->iSeek;
}


static void incrblobWatch(ClientData instanceData, int mode){ 
  /* NO-OP */ 
}
static int incrblobHandle(ClientData instanceData, int dir, ClientData *hPtr){
  return TCL_ERROR;
}

static Tcl_ChannelType IncrblobChannelType = {
  "incrblob",                        /* typeName                             */
  TCL_CHANNEL_VERSION_2,             /* version                              */
  incrblobClose,                     /* closeProc                            */
  incrblobInput,                     /* inputProc                            */
  incrblobOutput,                    /* outputProc                           */
  incrblobSeek,                      /* seekProc                             */
  0,                                 /* setOptionProc                        */
  0,                                 /* getOptionProc                        */
  incrblobWatch,                     /* watchProc (this is a no-op)          */
  incrblobHandle,                    /* getHandleProc (always returns error) */
  0,                                 /* close2Proc                           */
  0,                                 /* blockModeProc                        */
  0,                                 /* flushProc                            */
  0,                                 /* handlerProc                          */
  0,                                 /* wideSeekProc                         */
};

/*
** Create a new incrblob channel.
*/
int createIncrblobChannel(
  Tcl_Interp *interp, 
  dbi_Sqlite3_Data *pDb, 
  const char *zDb,
  const char *zTable, 
  const char *zColumn, 
  sqlite_int64 iRow,
  int isReadonly
){
  IncrblobChannel *p;
  sqlite3 *db = pDb->db;
  sqlite3_blob *pBlob;
  int rc;
  int flags = TCL_READABLE|(isReadonly ? 0 : TCL_WRITABLE);

  /* This variable is used to name the channels: "incrblob_[incr count]" */
  static int count = 0;
  char zChannel[64];

  rc = sqlite3_blob_open(db, zDb, zTable, zColumn, iRow, !isReadonly, &pBlob);
  if( rc!=SQLITE_OK ){
    Tcl_SetResult(interp, (char *)sqlite3_errmsg(pDb->db), TCL_VOLATILE);
    return TCL_ERROR;
  }

  p = (IncrblobChannel *)Tcl_Alloc(sizeof(IncrblobChannel));
  p->iSeek = 0;
  p->pBlob = pBlob;

  sqlite3_snprintf(sizeof(zChannel), zChannel, "incrblob_%d", ++count);
  p->channel = Tcl_CreateChannel(&IncrblobChannelType, zChannel, p, flags);
  Tcl_RegisterChannel(interp, p->channel);

  /* Link the new channel into the dbi_Sqlite3_Data.pIncrblob list. */
  p->pNext = pDb->pIncrblob;
  p->pPrev = 0;
  if( p->pNext ){
    p->pNext->pPrev = p;
  }
  pDb->pIncrblob = p;
  p->pDb = pDb;

  Tcl_SetResult(interp, (char *)Tcl_GetChannelName(p->channel), TCL_VOLATILE);
  return TCL_OK;
}

