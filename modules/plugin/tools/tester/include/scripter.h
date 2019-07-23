




































#ifndef __SCRIPTER_H__
#define __SCRIPTER_H__

#include "plugbase.h"
#include "action.h"
#include "script.h"

class CScripter
{
private:
  CPluginBase * m_pPlugin;
  CScriptItemList * m_pScript;
  BOOL m_bStopAutoExecFlag;
  int m_iCycleRepetitions;
  DWORD m_dwCycleDelay;

private:
  BOOL executeScriptItem(ScriptItemStruct * psis);

public:
  CScripter();
  ~CScripter();

  void associate(CPluginBase * pPlugin);
  void clearScript();

  BOOL getStopAutoExecFlag();
  void setStopAutoExecFlag(BOOL bFlag);

  int getCycleRepetitions();
  DWORD getCycleDelay();

  BOOL createScriptFromFile(LPSTR szFileName);
  BOOL executeScript();
};

#endif 
