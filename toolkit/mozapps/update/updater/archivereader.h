





#ifndef ArchiveReader_h__
#define ArchiveReader_h__

#include "mozilla/NullPtr.h"
#include <stdio.h>
#include "mar.h"

#ifdef XP_WIN
  typedef WCHAR NS_tchar;
#else
  typedef char NS_tchar;
#endif


class ArchiveReader
{
public:
  ArchiveReader() : mArchive(nullptr) {}
  ~ArchiveReader() { Close(); }

  int Open(const NS_tchar *path);
  int VerifySignature();
  int VerifyProductInformation(const char *MARChannelID, 
                               const char *appVersion);
  void Close();

  int ExtractFile(const char *item, const NS_tchar *destination);
  int ExtractFileToStream(const char *item, FILE *fp);

private:
  int ExtractItemToStream(const MarItem *item, FILE *fp);

  MarFile *mArchive;
};

#endif  
