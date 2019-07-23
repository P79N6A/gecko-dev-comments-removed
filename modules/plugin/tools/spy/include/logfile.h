




































#ifndef __LOGFILE_H__
#define __LOGFILE_H__

class CLogFile
{
private:
  char szFileName[256];
  XP_HFILE hFile;

public:
  CLogFile();
  ~CLogFile();

  BOOL create(char * filename, BOOL delete_existing = FALSE);
  void close();
  DWORD write(char * buf);
  void flush();
};

#endif 
