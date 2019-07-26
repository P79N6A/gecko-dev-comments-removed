




#ifndef NSDOMMEDIASTREAM_H_
#define NSDOMMEDIASTREAM_H_

#include "nsIDOMMediaStream.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIPrincipal.h"
#include "nsWrapperCache.h"
#include "nsIDOMWindow.h"

class nsXPCClassInfo;




#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

#ifdef CurrentTime
#undef CurrentTime
#endif

namespace mozilla {

class MediaStream;




class DOMMediaStream : public nsIDOMMediaStream,
                       public nsWrapperCache
{
  friend class DOMLocalMediaStream;

public:
  DOMMediaStream() : mStream(nullptr), mHintContents(0)
  {
    SetIsDOMBinding();
  }
  virtual ~DOMMediaStream();

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMMediaStream)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  nsIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);

  double CurrentTime();
  MediaStream* GetStream() { return mStream; }
  bool IsFinished();
  



  nsIPrincipal* GetPrincipal() { return mPrincipal; }

  





  bool CombineWithPrincipal(nsIPrincipal* aPrincipal);

  


  static already_AddRefed<DOMMediaStream>
  CreateSourceStream(nsIDOMWindow* aWindow, uint32_t aHintContents);

  
  
  enum {
    HINT_CONTENTS_AUDIO = 0x00000001U,
    HINT_CONTENTS_VIDEO = 0x00000002U
  };
  uint32_t GetHintContents() const { return mHintContents; }
  void SetHintContents(uint32_t aHintContents) { mHintContents = aHintContents; }

  


  static already_AddRefed<DOMMediaStream>
  CreateTrackUnionStream(nsIDOMWindow* aWindow, uint32_t aHintContents = 0);

protected:
  void InitSourceStream(nsIDOMWindow* aWindow, uint32_t aHintContents);
  void InitTrackUnionStream(nsIDOMWindow* aWindow, uint32_t aHintContents);

  
  nsCOMPtr<nsIDOMWindow> mWindow;

  
  
  MediaStream* mStream;
  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  
  uint32_t mHintContents;
};

class DOMLocalMediaStream : public DOMMediaStream,
                            public nsIDOMLocalMediaStream
{
public:
  DOMLocalMediaStream() {}
  virtual ~DOMLocalMediaStream();

  NS_DECL_ISUPPORTS_INHERITED

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);

  virtual void Stop();

  


  static already_AddRefed<DOMLocalMediaStream>
  CreateSourceStream(nsIDOMWindow* aWindow, uint32_t aHintContents);

  


  static already_AddRefed<DOMLocalMediaStream>
  CreateTrackUnionStream(nsIDOMWindow* aWindow, uint32_t aHintContents = 0);
};

}

#endif 
