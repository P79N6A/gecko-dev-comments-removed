




#ifndef mozilla_net_MemoryDownloader_h__
#define mozilla_net_MemoryDownloader_h__

#include "mozilla/UniquePtr.h"
#include "nsCOMPtr.h"
#include "nsIStreamListener.h"
#include "nsTArray.h"










namespace mozilla {
namespace net {

class MemoryDownloader final : public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  typedef mozilla::UniquePtr<FallibleTArray<uint8_t>> Data;

  class IObserver : public nsISupports {
  public:
    
    virtual void OnDownloadComplete(MemoryDownloader* aDownloader,
                                    nsIRequest* aRequest,
                                    nsISupports* aCtxt,
                                    nsresult aStatus,
                                    Data aData) = 0;
  };

  explicit MemoryDownloader(IObserver* aObserver);

private:
  virtual ~MemoryDownloader();

  static NS_METHOD ConsumeData(nsIInputStream *in,
                               void           *closure,
                               const char     *fromRawSegment,
                               uint32_t        toOffset,
                               uint32_t        count,
                               uint32_t       *writeCount);

  nsRefPtr<IObserver> mObserver;
  Data mData;
  nsresult mStatus;
};

} 
} 

#endif 
