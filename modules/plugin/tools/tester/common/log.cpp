




































#include "xp.h"

#include "log.h"





CLogItemList::CLogItemList() :
  m_pFirst(NULL),
  m_pLast(NULL),
  m_iCount(0)
{
}

CLogItemList::~CLogItemList()
{
  while(m_pFirst != NULL)
  {
    LogItemListElement * plile = m_pFirst;
    m_pFirst = plile->pNext;
    delete plile;
  }
}

int CLogItemList::add(LogItemStruct * plis)
{
  LogItemListElement *plile = new LogItemListElement;

  if(plile == NULL)
    return -1;

  plile->plis = plis;

  if(m_pFirst == NULL)
  {
    m_pFirst = plile;
    plile->pPrev = NULL;
  }
  else
  {
    m_pLast->pNext = plile;
    plile->pPrev = m_pLast;
  }

  plile->pNext = NULL;
  m_pLast = plile;
  m_iCount++;

  int iRet = m_iCount;
  return iRet;
}
