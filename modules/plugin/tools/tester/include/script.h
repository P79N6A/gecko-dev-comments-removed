




































#ifndef __SCRIPT_HPP__
#define __SCRIPT_HPP__

#include "plugbase.h"
#include "action.h"

struct ScriptArgumentStruct
{
  DWORD dwArg;
  int iLength;

  ScriptArgumentStruct()
  {
    iLength = 0;
  }

  ~ScriptArgumentStruct()
  {
    if(iLength != 0)
      delete [] (char *)dwArg;
    iLength = 0;
  }
};

struct ScriptItemStruct
{
  NPAPI_Action action;
  ScriptArgumentStruct arg1;
  ScriptArgumentStruct arg2;
  ScriptArgumentStruct arg3;
  ScriptArgumentStruct arg4;
  ScriptArgumentStruct arg5;
  ScriptArgumentStruct arg6;
  ScriptArgumentStruct arg7;
  DWORD dwDelay; 

  ScriptItemStruct()
  {
  }

  ~ScriptItemStruct()
  {
  }
};

struct ScriptItemListElement
{
  ScriptItemStruct * psis;
  ScriptItemListElement * pPrev;
  ScriptItemListElement * pNext;

  ScriptItemListElement()
  {
    psis = NULL;
    pPrev = NULL;
    pNext = NULL;
  }

  ~ScriptItemListElement()
  {
    if(psis != NULL)
      delete psis;
  }
};

class CScriptItemList
{
public:
  ScriptItemListElement * m_pFirst;
  ScriptItemListElement * m_pLast;
  int m_iCount;

public:
  CScriptItemList();
  ~CScriptItemList();

  int add(ScriptItemStruct * psis);
};

#endif 
