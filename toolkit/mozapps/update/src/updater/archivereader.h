





































#ifndef ArchiveReader_h__
#define ArchiveReader_h__

#include <stdio.h>
#include "mar.h"

#ifdef XP_WIN
# define NS_tchar WCHAR
#else
# define NS_tchar char
#endif


class ArchiveReader
{
public:
  ArchiveReader() : mArchive(NULL) {}
  ~ArchiveReader() { Close(); }

  int Open(const NS_tchar *path);
  void Close();

  int ExtractFile(const char *item, const NS_tchar *destination);
  int ExtractFileToStream(const char *item, FILE *fp);

private:
  int ExtractItemToStream(const MarItem *item, FILE *fp);

  MarFile *mArchive;
};

#endif  
