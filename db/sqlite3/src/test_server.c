









































































































































































































#if defined(SQLITE_SERVER) && !defined(SQLITE_OMIT_SHARED_CACHE)
#if defined(OS_UNIX) && OS_UNIX && defined(THREADSAFE) && THREADSAFE




#include <pthread.h>
#include "sqlite3.h"





typedef struct SqlMessage SqlMessage;
struct SqlMessage {
  int op;                      
  sqlite3 *pDb;                
  sqlite3_stmt *pStmt;         
  int errCode;                 
  const char *zIn;             
  int nByte;                   
  const char *zOut;            
  SqlMessage *pNext;           
  SqlMessage *pPrev;           
  pthread_mutex_t clientMutex; 
  pthread_cond_t clientWakeup; 
};




#define MSG_Open       1  /* sqlite3_open(zIn, &pDb) */
#define MSG_Prepare    2  /* sqlite3_prepare(pDb, zIn, nByte, &pStmt, &zOut) */
#define MSG_Step       3  /* sqlite3_step(pStmt) */
#define MSG_Reset      4  /* sqlite3_reset(pStmt) */
#define MSG_Finalize   5  /* sqlite3_finalize(pStmt) */
#define MSG_Close      6  /* sqlite3_close(pDb) */
#define MSG_Done       7  /* Server has finished with this message */






static struct ServerState {
  pthread_mutex_t queueMutex;   
  pthread_mutex_t serverMutex;  
  pthread_cond_t serverWakeup;  
  volatile int serverHalt;      
  SqlMessage *pQueueHead;       
  SqlMessage *pQueueTail;       
} g = {
  PTHREAD_MUTEX_INITIALIZER,
  PTHREAD_MUTEX_INITIALIZER,
  PTHREAD_COND_INITIALIZER,
};








static void sendToServer(SqlMessage *pMsg){
  

  pthread_mutex_init(&pMsg->clientMutex, 0);
  pthread_cond_init(&pMsg->clientWakeup, 0);

  

  pthread_mutex_lock(&g.queueMutex);
  pMsg->pNext = g.pQueueHead;
  if( g.pQueueHead==0 ){
    g.pQueueTail = pMsg;
  }else{
    g.pQueueHead->pPrev = pMsg;
  }
  pMsg->pPrev = 0;
  g.pQueueHead = pMsg;
  pthread_mutex_unlock(&g.queueMutex);

  


  pthread_mutex_lock(&pMsg->clientMutex);
  pthread_cond_signal(&g.serverWakeup);
  while( pMsg->op!=MSG_Done ){
    pthread_cond_wait(&pMsg->clientWakeup, &pMsg->clientMutex);
  }
  pthread_mutex_unlock(&pMsg->clientMutex);

  

  pthread_mutex_destroy(&pMsg->clientMutex);
  pthread_cond_destroy(&pMsg->clientWakeup);
}


























int sqlite3_client_open(const char *zDatabaseName, sqlite3 **ppDb){
  SqlMessage msg;
  msg.op = MSG_Open;
  msg.zIn = zDatabaseName;
  sendToServer(&msg);
  *ppDb = msg.pDb;
  return msg.errCode;
}
int sqlite3_client_prepare(
  sqlite3 *pDb,
  const char *zSql,
  int nByte,
  sqlite3_stmt **ppStmt,
  const char **pzTail
){
  SqlMessage msg;
  msg.op = MSG_Prepare;
  msg.pDb = pDb;
  msg.zIn = zSql;
  msg.nByte = nByte;
  sendToServer(&msg);
  *ppStmt = msg.pStmt;
  if( pzTail ) *pzTail = msg.zOut;
  return msg.errCode;
}
int sqlite3_client_step(sqlite3_stmt *pStmt){
  SqlMessage msg;
  msg.op = MSG_Step;
  msg.pStmt = pStmt;
  sendToServer(&msg);
  return msg.errCode;
}
int sqlite3_client_reset(sqlite3_stmt *pStmt){
  SqlMessage msg;
  msg.op = MSG_Reset;
  msg.pStmt = pStmt;
  sendToServer(&msg);
  return msg.errCode;
}
int sqlite3_client_finalize(sqlite3_stmt *pStmt){
  SqlMessage msg;
  msg.op = MSG_Finalize;
  msg.pStmt = pStmt;
  sendToServer(&msg);
  return msg.errCode;
}
int sqlite3_client_close(sqlite3 *pDb){
  SqlMessage msg;
  msg.op = MSG_Close;
  msg.pDb = pDb;
  sendToServer(&msg);
  return msg.errCode;
}








void *sqlite3_server(void *NotUsed){
  sqlite3_enable_shared_cache(1);
  if( pthread_mutex_trylock(&g.serverMutex) ){
    sqlite3_enable_shared_cache(0);
    return 0;  
  }
  while( !g.serverHalt ){
    SqlMessage *pMsg;

    

    pthread_mutex_lock(&g.queueMutex);
    while( g.pQueueTail==0 && g.serverHalt==0 ){
      pthread_cond_wait(&g.serverWakeup, &g.queueMutex);
    }
    pMsg = g.pQueueTail;
    if( pMsg ){
      if( pMsg->pPrev ){
        pMsg->pPrev->pNext = 0;
      }else{
        g.pQueueHead = 0;
      }
      g.pQueueTail = pMsg->pPrev;
    }
    pthread_mutex_unlock(&g.queueMutex);
    if( pMsg==0 ) break;

    

    pthread_mutex_lock(&pMsg->clientMutex);
    switch( pMsg->op ){
      case MSG_Open: {
        pMsg->errCode = sqlite3_open(pMsg->zIn, &pMsg->pDb);
        break;
      }
      case MSG_Prepare: {
        pMsg->errCode = sqlite3_prepare(pMsg->pDb, pMsg->zIn, pMsg->nByte,
                                        &pMsg->pStmt, &pMsg->zOut);
        break;
      }
      case MSG_Step: {
        pMsg->errCode = sqlite3_step(pMsg->pStmt);
        break;
      }
      case MSG_Reset: {
        pMsg->errCode = sqlite3_reset(pMsg->pStmt);
        break;
      }
      case MSG_Finalize: {
        pMsg->errCode = sqlite3_finalize(pMsg->pStmt);
        break;
      }
      case MSG_Close: {
        pMsg->errCode = sqlite3_close(pMsg->pDb);
        break;
      }
    }

    

    pMsg->op = MSG_Done;
    pthread_mutex_unlock(&pMsg->clientMutex);
    pthread_cond_signal(&pMsg->clientWakeup);
  }
  pthread_mutex_unlock(&g.serverMutex);
  sqlite3_thread_cleanup();
  return 0;
}






void sqlite3_server_start(void){
  pthread_t x;
  int rc;
  g.serverHalt = 0;
  rc = pthread_create(&x, 0, sqlite3_server, 0);
  if( rc==0 ){
    pthread_detach(x);
  }
}








void sqlite3_server_stop(void){
  g.serverHalt = 1;
  pthread_cond_broadcast(&g.serverWakeup);
}

#endif 
#endif 
