




#ifndef mozilla_dom_WebVTTLoadListener_h
#define mozilla_dom_WebVTTLoadListener_h

#include "nsIStreamListener.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsAutoPtr.h"
#include "nsAutoRef.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace dom {

class HTMLTrackElement;


















class WebVTTLoadListener MOZ_FINAL : public nsIStreamListener,
                                     public nsIChannelEventSink,
                                     public nsIInterfaceRequestor
{
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(WebVTTLoadListener,
                                           nsIStreamListener)

public:
  WebVTTLoadListener(HTMLTrackElement* aElement);
  ~WebVTTLoadListener();

  
  
  nsresult LoadResource();

private:
  static NS_METHOD ParseChunk(nsIInputStream* aInStream, void* aClosure,
                              const char* aFromSegment, uint32_t aToOffset,
                              uint32_t aCount, uint32_t* aWriteCount);

  nsRefPtr<HTMLTrackElement> mElement;
};



} 
} 

#endif 
