





































#ifndef ArchiveReader_h__
#define ArchiveReader_h__

#include <stdio.h>
#include "mar.h"


class ArchiveReader
{
public:
  ArchiveReader() : mArchive(NULL) {}
  ~ArchiveReader() { Close(); }

  int Open(const char *path);
  void Close();

  int ExtractFile(const char *item, const char *destination);

private:
  int ExtractItemToStream(const MarItem *item, FILE *fp);

  MarFile *mArchive;
};

#endif  
