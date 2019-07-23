





































#include "wtypes.h"
#include "nsArchiveExtractor.h"

nsArchiveExtractor::nsArchiveExtractor(const WCHAR *sArchiveName, DWORD dwSfxStubSize, nsExtractorProgress *pProgress) :
  m_sArchiveName(NULL), m_pProgress(pProgress), m_dwSfxStubSize(dwSfxStubSize)
{
  if (sArchiveName)
    m_sArchiveName = wcsdup(sArchiveName);
}

nsArchiveExtractor::~nsArchiveExtractor()
{
  free(m_sArchiveName);
}

int nsArchiveExtractor::Extract(const WCHAR *sDestinationDir)
{
  return OK;
}

int nsArchiveExtractor::ExtractFile(const WCHAR *sFileName, const WCHAR *sDestinationDir)
{
  return OK;
}
