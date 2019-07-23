





































#pragma once




class nsExtractorProgress
{
public:
  nsExtractorProgress() {}
  virtual void Progress(int nPercentComplete) = 0; 
};





class nsArchiveExtractor
{
public:
  enum
  {
    OK = 0,
    ERR_FAIL = -1,
    ERR_PARAM = -2,
    ERR_READ = -3,
    ERR_WRITE = -4,
    ERR_MEM = -5,
    ERR_NO_ARCHIVE = -6,
    ERR_EXTRACTION = -7,
  };

  nsArchiveExtractor(const WCHAR *sArchiveName, DWORD dwSfxStubSize, nsExtractorProgress *pProgress);
  virtual ~nsArchiveExtractor();

  virtual int Extract(const WCHAR *sDestinationDir);
  virtual int ExtractFile(const WCHAR *sFileName, const WCHAR *sDestinationDir);

protected:
  WCHAR *m_sArchiveName;
  nsExtractorProgress *m_pProgress;
  DWORD m_dwSfxStubSize;
};
