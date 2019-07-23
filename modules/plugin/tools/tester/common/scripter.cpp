




































#include "xp.h"

#include "scripter.h"
#include "scrpthlp.h"
#include "profile.h"





CScripter::CScripter() :
  m_pPlugin(NULL),
  m_pScript(NULL),
  m_bStopAutoExecFlag(FALSE),
  m_iCycleRepetitions(1),
  m_dwCycleDelay(0L)
{
  m_pScript = new CScriptItemList();
}

CScripter::~CScripter()
{
  if(m_pScript != NULL)
    delete m_pScript;
}
  
void CScripter::associate(CPluginBase * pPlugin)
{
  m_pPlugin = pPlugin;
}
  
BOOL CScripter::getStopAutoExecFlag()
{
  return m_bStopAutoExecFlag;
}

void CScripter::setStopAutoExecFlag(BOOL bFlag)
{
  m_bStopAutoExecFlag = bFlag;
}

int CScripter::getCycleRepetitions()
{
  return m_iCycleRepetitions;
}

DWORD CScripter::getCycleDelay()
{
  return m_dwCycleDelay;
}

void CScripter::clearScript()
{
  if(m_pScript != NULL)
    delete m_pScript;

  m_pScript = new CScriptItemList();
}

BOOL CScripter::executeScript()
{
  if(m_pScript == NULL)
    return FALSE;

  for(ScriptItemListElement * psile = m_pScript->m_pFirst; psile != NULL; psile = psile->pNext)
  {
    executeScriptItem(psile->psis);
  }

  return TRUE;
}

BOOL CScripter::createScriptFromFile(LPSTR szFileName)
{
  clearScript();

  if(!XP_IsFile(szFileName))
    return FALSE;

  char sz[16];

  XP_GetPrivateProfileString(SECTION_OPTIONS, KEY_LAST, PROFILE_DEFAULT_STRING, sz, sizeof(sz), szFileName);
  if(strcmp(sz, PROFILE_DEFAULT_STRING) == 0)
    return FALSE;

  int iLast = atoi(sz);

  int iFirst = 1;

  XP_GetPrivateProfileString(SECTION_OPTIONS, KEY_FIRST, PROFILE_DEFAULT_STRING, sz, sizeof(sz), szFileName);
  if(strcmp(sz, PROFILE_DEFAULT_STRING) != 0)
    iFirst = atoi(sz);

  XP_GetPrivateProfileString(SECTION_OPTIONS, KEY_DELAY, PROFILE_DEFAULT_STRING, sz, sizeof(sz), szFileName);
  if(strcmp(sz, PROFILE_DEFAULT_STRING) == 0)
    m_dwCycleDelay = 0L;
  else
    m_dwCycleDelay = atol(sz);

  XP_GetPrivateProfileString(SECTION_OPTIONS, KEY_REPETITIONS, PROFILE_DEFAULT_STRING, sz, sizeof(sz), szFileName);
  if(strcmp(sz, PROFILE_DEFAULT_STRING) == 0)
    m_iCycleRepetitions = 1;
  else
    m_iCycleRepetitions = atoi(sz);

  for(int i = iFirst; i <= iLast; i++)
  {
    char szSection[16];
    wsprintf(szSection, "%i", i);
    ScriptItemStruct * psis = readProfileSectionAndCreateScriptItemStruct(szFileName, szSection);
    if(psis == NULL)
      continue;
    m_pScript->add(psis);
  }
  return TRUE;
}

BOOL CScripter::executeScriptItem(ScriptItemStruct * psis)
{
  if(psis == NULL)
    return FALSE;
  if(m_pPlugin == NULL)
    return FALSE;

  DWORD dw1 = psis->arg1.dwArg;
  DWORD dw2 = psis->arg2.dwArg;
  DWORD dw3 = psis->arg3.dwArg;
  DWORD dw4 = psis->arg4.dwArg;
  DWORD dw5 = psis->arg5.dwArg;
  DWORD dw6 = psis->arg6.dwArg;
  DWORD dw7 = psis->arg7.dwArg;

  if(psis->action == action_npn_get_value)
  {
    static DWORD dwValue = 0L;
    m_pPlugin->m_pValue = (void *)&dwValue;
  }

  m_pPlugin->makeNPNCall(psis->action, dw1, dw2, dw3, dw4, dw5, dw6, dw7);

  if(psis->dwDelay >=0)
    XP_Sleep(psis->dwDelay);

  return TRUE;
}
