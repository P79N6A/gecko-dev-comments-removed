




































#include "xp.h"

#include "script.h"
#include "scrpthlp.h"

CScriptItemList::CScriptItemList() :
  m_pFirst(NULL),
  m_pLast(NULL),
  m_iCount(0)
{
}

CScriptItemList::~CScriptItemList()
{
  while(m_pFirst != NULL)
  {
    ScriptItemListElement * psile = m_pFirst;
    m_pFirst = psile->pNext;
    delete psile;
  }
}

int CScriptItemList::add(ScriptItemStruct * psis)
{
  ScriptItemListElement * psile = new ScriptItemListElement;

  if(psile == NULL)
    return -1;

  psile->psis = psis;

  if(m_pFirst == NULL)
  {
    m_pFirst = psile;
    psile->pPrev = NULL;
  }
  else
  {
    m_pLast->pNext = psile;
    psile->pPrev = m_pLast;
  }

  psile->pNext = NULL;
  m_pLast = psile;
  m_iCount++;

  int iRet = m_iCount;
  return iRet;
}
