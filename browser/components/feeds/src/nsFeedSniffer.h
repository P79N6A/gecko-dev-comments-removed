





































#include "nsIGenericFactory.h"
#include "nsIContentSniffer.h"
#include "nsIStreamListener.h"
#include "nsStringAPI.h"

class nsFeedSniffer : public nsIContentSniffer, nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTSNIFFER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  static NS_METHOD AppendSegmentToString(nsIInputStream* inputStream,
                                         void* closure,
                                         const char* rawSegment,
                                         PRUint32 toOffset,
                                         PRUint32 count,
                                         PRUint32* writeCount);

  static NS_METHOD Register(nsIComponentManager* compMgr, nsIFile* path, 
                            const char* registryLocation,
                            const char* componentType, 
                            const nsModuleComponentInfo *info);

protected:
  nsresult ConvertEncodedData(nsIRequest* request, const PRUint8* data,
                              PRUint32 length);

private:
  nsCString mDecodedData;
};

