




































#include "xp.h"

#include "logfile.h"

CLogFile::CLogFile() :
  m_hFile(NULL)
{
  m_szFileName[0] = '\0';
}

CLogFile::~CLogFile()
{
  if(m_hFile != NULL)
    close();
}

BOOL CLogFile::create(LPSTR szFileName, BOOL bDeleteExisting)
{
  strcpy(m_szFileName, szFileName);

  if(XP_IsFile(m_szFileName))
  {
    if(!bDeleteExisting)
      return FALSE;
  }

  m_hFile = XP_CreateFile(m_szFileName);
  return (m_hFile != NULL);
}

void CLogFile::close()
{
  if(m_hFile != NULL)
  {
    XP_CloseFile(m_hFile);
    m_hFile = NULL;
  }
}

DWORD CLogFile::write(LPSTR szBuf)
{
  return XP_WriteFile(m_hFile, szBuf, strlen(szBuf));
}

void CLogFile::flush()
{
  XP_FlushFileBuffers(m_hFile);
}

