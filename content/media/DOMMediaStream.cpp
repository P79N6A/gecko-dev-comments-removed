




#include "DOMMediaStream.h"
#include "nsDOMClassInfoID.h"
#include "nsContentUtils.h"
#include "mozilla/dom/MediaStreamBinding.h"
#include "mozilla/dom/LocalMediaStreamBinding.h"
#include "MediaStreamGraph.h"

using namespace mozilla;

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMMediaStream)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMediaStream)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMMediaStream)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMMediaStream)
NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(DOMMediaStream, mWindow)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMLocalMediaStream)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLocalMediaStream)
NS_INTERFACE_MAP_END_INHERITING(DOMMediaStream)

NS_IMPL_ADDREF_INHERITED(DOMLocalMediaStream, DOMMediaStream)
NS_IMPL_RELEASE_INHERITED(DOMLocalMediaStream, DOMMediaStream)
NS_IMPL_CYCLE_COLLECTION_INHERITED_0(DOMLocalMediaStream, DOMMediaStream)

DOMMediaStream::~DOMMediaStream()
{
  if (mStream) {
    mStream->Destroy();
  }
}

JSObject*
DOMMediaStream::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return dom::MediaStreamBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

double
DOMMediaStream::CurrentTime()
{
  return mStream ? MediaTimeToSeconds(mStream->GetCurrentTime()) : 0.0;
}

bool
DOMMediaStream::IsFinished()
{
  return !mStream || mStream->IsFinished();
}

void
DOMMediaStream::InitSourceStream(nsIDOMWindow* aWindow, uint32_t aHintContents)
{
  mWindow = aWindow;
  SetHintContents(aHintContents);
  MediaStreamGraph* gm = MediaStreamGraph::GetInstance();
  mStream = gm->CreateSourceStream(this);
}

void
DOMMediaStream::InitTrackUnionStream(nsIDOMWindow* aWindow, uint32_t aHintContents)
{
  mWindow = aWindow;
  SetHintContents(aHintContents);
  MediaStreamGraph* gm = MediaStreamGraph::GetInstance();
  mStream = gm->CreateTrackUnionStream(this);
}

already_AddRefed<DOMMediaStream>
DOMMediaStream::CreateSourceStream(nsIDOMWindow* aWindow, uint32_t aHintContents)
{
  nsRefPtr<DOMMediaStream> stream = new DOMMediaStream();
  stream->InitSourceStream(aWindow, aHintContents);
  return stream.forget();
}

already_AddRefed<DOMMediaStream>
DOMMediaStream::CreateTrackUnionStream(nsIDOMWindow* aWindow, uint32_t aHintContents)
{
  nsRefPtr<DOMMediaStream> stream = new DOMMediaStream();
  stream->InitTrackUnionStream(aWindow, aHintContents);
  return stream.forget();
}

bool
DOMMediaStream::CombineWithPrincipal(nsIPrincipal* aPrincipal)
{
  return nsContentUtils::CombineResourcePrincipals(&mPrincipal, aPrincipal);
}

DOMLocalMediaStream::~DOMLocalMediaStream()
{
  if (mStream) {
    
    Stop();
  }
}

JSObject*
DOMLocalMediaStream::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return dom::LocalMediaStreamBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

void
DOMLocalMediaStream::Stop()
{
  if (mStream && mStream->AsSourceStream()) {
    mStream->AsSourceStream()->EndAllTrackAndFinish();
  }
}

already_AddRefed<DOMLocalMediaStream>
DOMLocalMediaStream::CreateSourceStream(nsIDOMWindow* aWindow, uint32_t aHintContents)
{
  nsRefPtr<DOMLocalMediaStream> stream = new DOMLocalMediaStream();
  stream->InitSourceStream(aWindow, aHintContents);
  return stream.forget();
}

already_AddRefed<DOMLocalMediaStream>
DOMLocalMediaStream::CreateTrackUnionStream(nsIDOMWindow* aWindow, uint32_t aHintContents)
{
  nsRefPtr<DOMLocalMediaStream> stream = new DOMLocalMediaStream();
  stream->InitTrackUnionStream(aWindow, aHintContents);
  return stream.forget();
}
