




































#include "xp.h"
#include "logger.h"
#include "loghlp.h"

#ifdef XP_MAC
	CLogger *pLogger = new CLogger();
#endif





CLogger::CLogger(LPSTR szTarget) :
  m_pPlugin(NULL),
  m_pPluginInstance(NULL),
  m_pLog(NULL),
  m_pLogFile(NULL),
  m_bShowImmediately(FALSE),
  m_bLogToFile(FALSE),
  m_bLogToFrame(TRUE),
  m_bBlockLogToFile(TRUE),
  m_bBlockLogToFrame(FALSE),
  m_pStream(NULL),
  m_dwStartTime(0xFFFFFFFF),
  m_iStringDataWrap(LOGGER_DEFAULT_STRING_WRAP),
  m_bStale(FALSE)
{
  if(szTarget != NULL)
    strcpy(m_szTarget, szTarget);
  else
    strcpy(m_szTarget, "");

  m_pLog = new CLogItemList();
  strcpy(m_szStreamType, "text/plain");
}

CLogger::~CLogger()
{
  if(m_pLogFile != NULL)
    delete m_pLogFile;

  if(m_pLog != NULL)
    delete m_pLog;
}

void CLogger::associate(CPluginBase * pPlugin)
{
  m_pPlugin = pPlugin;
  m_pPluginInstance = m_pPlugin->getNPInstance();
}

void CLogger::restorePreferences(LPSTR szFileName)
{
  XP_GetPrivateProfileString(SECTION_LOG, KEY_RECORD_SEPARATOR, "", m_szItemSeparator, 
                             sizeof(m_szItemSeparator), szFileName);
  m_iStringDataWrap = XP_GetPrivateProfileInt(SECTION_LOG, KEY_STRING_WRAP, 
                                              LOGGER_DEFAULT_STRING_WRAP, szFileName);
}

BOOL CLogger::onNPP_DestroyStream(NPStream * npStream)
{
  if(npStream == m_pStream) 
  {
    m_pStream = NULL;
    return TRUE;
  }
  return FALSE;
}

static void FixUpOutputString(char * aString)
{
  
  char * p = aString;
  while((p = strstr(p, "<")))
    *p = '[';

  p = aString;
  while((p = strstr(p, ">")))
    *p = ']';
}

BOOL CLogger::appendToLog(NPAPI_Action action, DWORD dwTickEnter, DWORD dwTickReturn, 
                          DWORD dwRet, 
                          DWORD dw1, DWORD dw2, DWORD dw3, DWORD dw4, 
                          DWORD dw5, DWORD dw6, DWORD dw7)
{
  if(m_pLog == NULL)
    return FALSE;

  if(!m_bLogToFrame && !m_bLogToFile)
    return TRUE;

  DWORD dwTimeEnter;
  DWORD dwTimeReturn;
  if(m_dwStartTime == 0xFFFFFFFF)
  {
    m_dwStartTime = dwTickEnter;
    dwTimeEnter = 0L;
  }
  else
    dwTimeEnter = dwTickEnter - m_dwStartTime;

  dwTimeReturn = dwTickReturn - m_dwStartTime;

  LogItemStruct * plis = makeLogItemStruct(action, dwTimeEnter, dwTimeReturn, dwRet, 
                                           dw1, dw2, dw3, dw4, dw5, dw6, dw7);

  static char szOutput[1024];

  
  if(m_bLogToFile && !m_bBlockLogToFile)
  {
    if(m_pLogFile == NULL)
    {
      m_pLogFile = new CLogFile();
      if(m_pLogFile == NULL)
        return FALSE;

      char szFile[256];
      m_pPlugin->getLogFileName(szFile, sizeof(szFile));

      if(m_pPlugin->getMode() == NP_EMBED)
      {
        if(!m_pLogFile->create(szFile, FALSE))
        {
          char szMessage[512];
          wsprintf(szMessage, "File '%s'\n probably exists. Overwrite?", szFile);
          if(IDYES == m_pPlugin->messageBox(szMessage, "", MB_ICONQUESTION | MB_YESNO))
          {
            if(!m_pLogFile->create(szFile, TRUE))
            {
              m_pPlugin->messageBox("Cannot create file.", "", MB_ICONERROR | MB_OK);
              delete m_pLogFile;
              m_pLogFile = NULL;
              return FALSE;
            }
          }
          else
          {
            delete m_pLogFile;
            m_pLogFile = NULL;
            goto Frame;
          }
        }
      }
      else 
      {
        if(!m_pLogFile->create(szFile, TRUE))
        {
          delete m_pLogFile;
          m_pLogFile = NULL;
          return FALSE;
        }
      }
    }

    formatLogItem(plis, szOutput, m_szItemSeparator, TRUE);
    m_pLogFile->write(szOutput);
    m_pLogFile->flush();
  }

Frame:

  
  if(m_bLogToFrame && !m_bBlockLogToFrame)
  {
    if(m_bShowImmediately)
    {
      BOOL dosstyle = (m_pPlugin && m_pPlugin->isStandAlone());

      int iLength = formatLogItem(plis, szOutput, "", dosstyle);

      
      
      
      
      FixUpOutputString(szOutput);

      if (m_pPlugin && m_pPlugin->isStandAlone())
      {
        m_pPlugin->outputToNativeWindow(szOutput);
      }
      else
      {
        if(m_pStream == NULL)
          NPN_NewStream(m_pPluginInstance, m_szStreamType, m_szTarget, &m_pStream);

        NPN_Write(m_pPluginInstance, m_pStream, iLength, (void *)szOutput);
      }
      delete plis;
    }
    else
      m_pLog->add(plis);
  }

  return TRUE;
}

void CLogger::setShowImmediatelyFlag(BOOL bFlagState)
{
  m_bShowImmediately = bFlagState;
}

BOOL CLogger::getShowImmediatelyFlag()
{
  return m_bShowImmediately;
}

void CLogger::setLogToFileFlag(BOOL bFlagState)
{
  m_bLogToFile = bFlagState;
}

BOOL CLogger::getLogToFileFlag()
{
  return m_bLogToFile;
}

void CLogger::setLogToFrameFlag(BOOL bFlagState)
{
  m_bLogToFrame = bFlagState;
}

BOOL CLogger::getLogToFrameFlag()
{
  return m_bLogToFrame;
}

int CLogger::getStringDataWrap()
{
  return m_iStringDataWrap;
}

void CLogger::clearLog()
{
  if(m_pLog != NULL)
    delete m_pLog;

  m_pLog = new CLogItemList();
}

void CLogger::clearTarget()
{
  if (m_pPlugin && m_pPlugin->isStandAlone())
  {
    m_pPlugin->outputToNativeWindow("");
  }
  else
  {
    if(m_pStream != NULL)
      NPN_DestroyStream(m_pPluginInstance, m_pStream, NPRES_DONE);

    NPN_NewStream(m_pPluginInstance, m_szStreamType, m_szTarget, &m_pStream);
    NPN_Write(m_pPluginInstance, m_pStream, 1, (void *)"\n");

    if(!m_bShowImmediately)
    {
      NPN_DestroyStream(m_pPluginInstance, m_pStream, NPRES_DONE);
      m_pStream = NULL;
    }
  }
}

void CLogger::resetStartTime()
{
  m_dwStartTime = XP_GetTickCount();
}

void CLogger::dumpLogToTarget()
{
  if(m_pLog == NULL)
    return;

  static char szOutput[1024];

  if (m_pPlugin && m_pPlugin->isStandAlone())
  {
    for(LogItemListElement * plile = m_pLog->m_pFirst; plile != NULL; plile = plile->pNext)
    {
      formatLogItem(plile->plis, szOutput, "", TRUE);
      m_pPlugin->outputToNativeWindow(szOutput);
    }
  }
  else
  {
    BOOL bTemporaryStream = ((m_pStream == NULL) && !getShowImmediatelyFlag());

    if(m_pStream == NULL)
      NPN_NewStream(m_pPluginInstance, m_szStreamType, m_szTarget, &m_pStream);

    for(LogItemListElement * plile = m_pLog->m_pFirst; plile != NULL; plile = plile->pNext)
    {
      int iLength = formatLogItem(plile->plis, szOutput, "");
      NPN_Write(m_pPluginInstance, m_pStream, iLength, (void *)szOutput);
    }

    if(bTemporaryStream)
    {
      NPN_DestroyStream(m_pPluginInstance, m_pStream, NPRES_DONE);
      m_pStream = NULL;
    }
  }
}

void CLogger::closeLogToFile()
{
  if(m_pLogFile != NULL)
  {
    delete m_pLogFile;
    m_pLogFile = NULL;
  }
}

void CLogger::blockDumpToFile(BOOL bBlock)
{
  m_bBlockLogToFile = bBlock;
}

void CLogger::blockDumpToFrame(BOOL bBlock)
{
  m_bBlockLogToFrame = bBlock;
}

void CLogger::markStale()
{
  m_bStale = TRUE;
}

BOOL CLogger::isStale()
{
  return m_bStale;
}
