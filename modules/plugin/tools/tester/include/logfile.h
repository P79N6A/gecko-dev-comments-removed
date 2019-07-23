




































#ifndef __LOGFILE_H__
#define __LOGFILE_H__

#include "xp.h"

class CLogFile
{
private:
  char m_szFileName[256];
  XP_HFILE m_hFile;

public:
  CLogFile();
  ~CLogFile();

  BOOL create(LPSTR szFileName, BOOL bDeleteExisting = FALSE);
  void close();
  DWORD write(LPSTR szBuf);
  void flush();
};

#endif 
