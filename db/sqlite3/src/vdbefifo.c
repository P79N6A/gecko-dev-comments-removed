













#include "sqliteInt.h"
#include "vdbeInt.h"





static FifoPage *allocatePage(int nEntry){
  FifoPage *pPage;
  if( nEntry>32767 ){
    nEntry = 32767;
  }
  pPage = sqliteMallocRaw( sizeof(FifoPage) + sizeof(i64)*(nEntry-1) );
  if( pPage ){
    pPage->nSlot = nEntry;
    pPage->iWrite = 0;
    pPage->iRead = 0;
    pPage->pNext = 0;
  }
  return pPage;
}




void sqlite3VdbeFifoInit(Fifo *pFifo){
  memset(pFifo, 0, sizeof(*pFifo));
}






int sqlite3VdbeFifoPush(Fifo *pFifo, i64 val){
  FifoPage *pPage;
  pPage = pFifo->pLast;
  if( pPage==0 ){
    pPage = pFifo->pLast = pFifo->pFirst = allocatePage(20);
    if( pPage==0 ){
      return SQLITE_NOMEM;
    }
  }else if( pPage->iWrite>=pPage->nSlot ){
    pPage->pNext = allocatePage(pFifo->nEntry);
    if( pPage->pNext==0 ){
      return SQLITE_NOMEM;
    }
    pPage = pFifo->pLast = pPage->pNext;
  }
  pPage->aSlot[pPage->iWrite++] = val;
  pFifo->nEntry++;
  return SQLITE_OK;
}






int sqlite3VdbeFifoPop(Fifo *pFifo, i64 *pVal){
  FifoPage *pPage;
  if( pFifo->nEntry==0 ){
    return SQLITE_DONE;
  }
  assert( pFifo->nEntry>0 );
  pPage = pFifo->pFirst;
  assert( pPage!=0 );
  assert( pPage->iWrite>pPage->iRead );
  assert( pPage->iWrite<=pPage->nSlot );
  assert( pPage->iRead<pPage->nSlot );
  assert( pPage->iRead>=0 );
  *pVal = pPage->aSlot[pPage->iRead++];
  pFifo->nEntry--;
  if( pPage->iRead>=pPage->iWrite ){
    pFifo->pFirst = pPage->pNext;
    sqliteFree(pPage);
    if( pFifo->nEntry==0 ){
      assert( pFifo->pLast==pPage );
      pFifo->pLast = 0;
    }else{
      assert( pFifo->pFirst!=0 );
    }
  }else{
    assert( pFifo->nEntry>0 );
  }
  return SQLITE_OK;
}





void sqlite3VdbeFifoClear(Fifo *pFifo){
  FifoPage *pPage, *pNextPage;
  for(pPage=pFifo->pFirst; pPage; pPage=pNextPage){
    pNextPage = pPage->pNext;
    sqliteFree(pPage);
  }
  sqlite3VdbeFifoInit(pFifo);
}
