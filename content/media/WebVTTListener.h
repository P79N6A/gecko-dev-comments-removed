




#ifndef mozilla_dom_WebVTTLoadListener_h
#define mozilla_dom_WebVTTLoadListener_h

#include "nsIWebVTTListener.h"
#include "nsIStreamListener.h"
#include "nsIChannelEventSink.h"
#include "nsAutoPtr.h"
#include "nsIInterfaceRequestor.h"
#include "nsCycleCollectionParticipant.h"

class nsIWebVTTParserWrapper;

namespace mozilla {
namespace dom {

class HTMLTrackElement;

class WebVTTListener MOZ_FINAL : public nsIWebVTTListener,
                                 public nsIStreamListener,
                                 public nsIChannelEventSink,
                                 public nsIInterfaceRequestor
{
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIWEBVTTLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(WebVTTListener,
                                           nsIStreamListener)

public:
  explicit WebVTTListener(HTMLTrackElement* aElement);

  



  nsresult LoadResource();

private:
  ~WebVTTListener();

  
  enum ErrorCodes {
    BadSignature = 0
  };
  static NS_METHOD ParseChunk(nsIInputStream* aInStream, void* aClosure,
                              const char* aFromSegment, uint32_t aToOffset,
                              uint32_t aCount, uint32_t* aWriteCount);

  nsRefPtr<HTMLTrackElement> mElement;
  nsCOMPtr<nsIWebVTTParserWrapper> mParserWrapper;
};

} 
} 

#endif 
