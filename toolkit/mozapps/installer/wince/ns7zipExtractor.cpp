





































#include "wtypes.h"
#include "nsArchiveExtractor.h"
#include "ns7zipExtractor.h"
#include "7zLib.h"

static nsExtractorProgress *g_pProgress = NULL;

static void ExtractProgressCallback(int nPercentComplete)
{
  if (g_pProgress)
  {
    g_pProgress->Progress(nPercentComplete);
  }
}

static void ShowError()
{
  MessageBoxW(GetForegroundWindow(), GetExtractorError(), L"Extractor", MB_OK|MB_ICONERROR);
}

ns7zipExtractor::ns7zipExtractor(const WCHAR *sArchiveName, DWORD dwSfxStubSize, nsExtractorProgress *pProgress) :
  nsArchiveExtractor(sArchiveName, dwSfxStubSize, pProgress)
{
  g_pProgress = pProgress;
}

ns7zipExtractor::~ns7zipExtractor()
{
  g_pProgress = NULL;
}

int ns7zipExtractor::Extract(const WCHAR *sDestinationDir)
{
  int res = SzExtractSfx(m_sArchiveName, m_dwSfxStubSize, NULL, sDestinationDir, ExtractProgressCallback);
  if (res != 0)
    ShowError();
  return res;
}

int ns7zipExtractor::ExtractFile(const WCHAR *sFileName, const WCHAR *sDestinationDir)
{
  int res = SzExtractSfx(m_sArchiveName, m_dwSfxStubSize, sFileName, sDestinationDir, NULL);
  if (res != 0)
    ShowError();
  return res;
}
