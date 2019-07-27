





#ifndef nsGZFileWriter_h
#define nsGZFileWriter_h

#include "nsIGZFileWriter.h"
#include "zlib.h"




class nsGZFileWriter MOZ_FINAL : public nsIGZFileWriter
{
  virtual ~nsGZFileWriter();

public:
  nsGZFileWriter();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIGZFILEWRITER

  




  nsresult Write(const char* aStr)
  {
    return nsIGZFileWriter::Write(aStr);
  }

  nsresult Write(const char* aStr, uint32_t aLen)
  {
    return nsIGZFileWriter::Write(aStr, aLen);
  }

private:
  bool mInitialized;
  bool mFinished;
  gzFile mGZFile;
};

#endif
