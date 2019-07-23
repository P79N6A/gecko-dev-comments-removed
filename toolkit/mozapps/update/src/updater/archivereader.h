





































#ifndef ArchiveReader_h__
#define ArchiveReader_h__

#include <stdio.h>
#include "mar.h"


class ArchiveReader
{
public:
  ArchiveReader() : mArchive(NULL) {}
  ~ArchiveReader() { Close(); }

#ifdef XP_WIN
  int Open(const WCHAR *path);
#else
  int Open(const char *path);
#endif

  void Close();

  int ExtractFile(const char *item, const char *destination);
  int ExtractFileToStream(const char *item, FILE *fp);

private:
  int ExtractItemToStream(const MarItem *item, FILE *fp);

  MarFile *mArchive;
};

#endif  
