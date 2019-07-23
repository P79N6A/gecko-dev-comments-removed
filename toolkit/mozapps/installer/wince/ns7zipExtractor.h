





































#pragma once

#include "nsArchiveExtractor.h"

class ns7zipExtractor : public nsArchiveExtractor
{
public:
  ns7zipExtractor(const WCHAR *sArchiveName, DWORD dwSfxStubSize, nsExtractorProgress *pProgress);
  virtual ~ns7zipExtractor();

  virtual int Extract(const WCHAR *sDestinationDir);
  virtual int ExtractFile(const WCHAR *sFileName, const WCHAR *sDestinationDir);
};
