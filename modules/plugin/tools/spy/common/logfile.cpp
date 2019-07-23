




































#include "xp.h"

#include "logfile.h"
#include "plugload.h"

CLogFile::CLogFile() :
  hFile(NULL)
{
  szFileName[0] = '\0';
}

CLogFile::~CLogFile()
{
  if(hFile != NULL)
    close();
}

BOOL CLogFile::create(char * filename, BOOL delete_existing)
{
  strcpy(szFileName, filename);

  if(!delete_existing && XP_IsFile(szFileName))
      return FALSE;

  hFile = XP_CreateFile(szFileName);
  return (hFile != NULL);
}

void CLogFile::close()
{
  if(hFile != NULL)
  {
    XP_CloseFile(hFile);
    hFile = NULL;
  }
}

DWORD CLogFile::write(char * buf)
{
  return XP_WriteFile(hFile, buf, strlen(buf));
}

void CLogFile::flush()
{
  XP_FlushFileBuffers(hFile);
}
