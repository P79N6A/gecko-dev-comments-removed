





#include "nsIGZFileWriter.h"
#include <stdio.h>
#include "zlib.h"




class nsGZFileWriter : public nsIGZFileWriter
{
public:
  nsGZFileWriter();
  virtual ~nsGZFileWriter();

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
