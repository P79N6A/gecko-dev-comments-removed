




































#ifndef __LOG_HPP__
#define __LOG_HPP__

#include "action.h"

struct LogArgumentStruct
{
  DWORD dwArg;
  int iLength;
  void * pData;

  LogArgumentStruct()
  {
    iLength = 0;
    pData = NULL;
  }

  ~LogArgumentStruct()
  {
    if(pData != NULL)
      delete [] pData;
    iLength = 0;
  }
};

struct LogItemStruct
{
  NPAPI_Action action;
  DWORD dwReturnValue;
  LogArgumentStruct arg1;
  LogArgumentStruct arg2;
  LogArgumentStruct arg3;
  LogArgumentStruct arg4;
  LogArgumentStruct arg5;
  LogArgumentStruct arg6;
  LogArgumentStruct arg7;
  DWORD dwTimeEnter;
  DWORD dwTimeReturn;

  LogItemStruct()
  {
  }

  ~LogItemStruct()
  {
  }
};

struct LogItemListElement
{
  LogItemStruct * plis;
  LogItemListElement * pPrev;
  LogItemListElement * pNext;

  LogItemListElement()
  {
    plis = NULL;
    pPrev = NULL;
    pNext = NULL;
  }

  ~LogItemListElement()
  {
    if(plis != NULL)
      delete plis;
  }
};

class CLogItemList
{
public:
  LogItemListElement * m_pFirst;
  LogItemListElement * m_pLast;
  int m_iCount;

public:
  CLogItemList();
  ~CLogItemList();

  int add(LogItemStruct * plis);
};

#endif 
