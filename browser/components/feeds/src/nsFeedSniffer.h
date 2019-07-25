





#include "nsIContentSniffer.h"
#include "nsIStreamListener.h"
#include "nsStringAPI.h"
#include "mozilla/Attributes.h"

class nsFeedSniffer MOZ_FINAL : public nsIContentSniffer,
                                       nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTSNIFFER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  static NS_METHOD AppendSegmentToString(nsIInputStream* inputStream,
                                         void* closure,
                                         const char* rawSegment,
                                         uint32_t toOffset,
                                         uint32_t count,
                                         uint32_t* writeCount);

protected:
  nsresult ConvertEncodedData(nsIRequest* request, const uint8_t* data,
                              uint32_t length);

private:
  nsCString mDecodedData;
};

