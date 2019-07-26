



#ifndef MediaStreamList_h__
#define MediaStreamList_h__

#include "mozilla/ErrorResult.h"
#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"
#include "jspubtd.h"
#include "mozilla/dom/NonRefcountedDOMObject.h"

class nsIDOMMediaStream;
namespace sipcc {
class PeerConnectionImpl;
} 

namespace mozilla {
namespace dom {

class MediaStreamList : public NonRefcountedDOMObject
{
public:
  enum StreamType {
    Local,
    Remote
  };

  MediaStreamList(sipcc::PeerConnectionImpl* peerConnection, StreamType type);
  ~MediaStreamList();

  JSObject* WrapObject(JSContext* cx, ErrorResult& error);

  nsIDOMMediaStream* IndexedGetter(uint32_t index, bool& found);
  uint32_t Length();

private:
  nsRefPtr<sipcc::PeerConnectionImpl> mPeerConnection;
  StreamType mType;
};

} 
} 

#endif
