





#ifndef __Helpers_h
#define __Helpers_h

#include "nsIAsyncOutputStream.h"
#include "nsString.h"
#include <stdint.h>

class nsIInputStream;
class nsIOutputStream;
template <class T> class nsTArray;

namespace testing {

void
CreateData(uint32_t aNumBytes, nsTArray<char>& aDataOut);

void
Write(nsIOutputStream* aStream, const nsTArray<char>& aData, uint32_t aOffset,
      uint32_t aNumBytes);

void
WriteAllAndClose(nsIOutputStream* aStream, const nsTArray<char>& aData);

void
ConsumeAndValidateStream(nsIInputStream* aStream,
                         const nsTArray<char>& aExpectedData);

void
ConsumeAndValidateStream(nsIInputStream* aStream,
                         const nsACString& aExpectedData);

class OutputStreamCallback MOZ_FINAL : public nsIOutputStreamCallback
{
public:
  OutputStreamCallback();

  bool Called() const { return mCalled; }

private:
  ~OutputStreamCallback();

  bool mCalled;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAMCALLBACK
};

} 

#endif
