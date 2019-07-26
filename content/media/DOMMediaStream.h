




#ifndef NSDOMMEDIASTREAM_H_
#define NSDOMMEDIASTREAM_H_

#include "nsIDOMMediaStream.h"
#include "MediaStreamGraph.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIPrincipal.h"

class nsXPCClassInfo;




#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

namespace mozilla {




class DOMMediaStream : public nsIDOMMediaStream
{
  friend class DOMLocalMediaStream;

public:
  DOMMediaStream() : mStream(nullptr), mHintContents(0) {}
  virtual ~DOMMediaStream();

  NS_DECL_CYCLE_COLLECTION_CLASS(DOMMediaStream)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_NSIDOMMEDIASTREAM

  MediaStream* GetStream() { return mStream; }
  bool IsFinished() { return !mStream || mStream->IsFinished(); }
  



  nsIPrincipal* GetPrincipal() { return mPrincipal; }

  





  bool CombineWithPrincipal(nsIPrincipal* aPrincipal);

  


  static already_AddRefed<DOMMediaStream> CreateSourceStream(uint32_t aHintContents);

  
  
  enum {
    HINT_CONTENTS_AUDIO = 0x00000001U,
    HINT_CONTENTS_VIDEO = 0x00000002U
  };
  uint32_t GetHintContents() const { return mHintContents; }
  void SetHintContents(uint32_t aHintContents) { mHintContents = aHintContents; }

  


  static already_AddRefed<DOMMediaStream> CreateTrackUnionStream(uint32_t aHintContents = 0);

protected:
  void InitSourceStream(uint32_t aHintContents)
  {
    SetHintContents(aHintContents);
    MediaStreamGraph* gm = MediaStreamGraph::GetInstance();
    mStream = gm->CreateSourceStream(this);
  }
  void InitTrackUnionStream(uint32_t aHintContents)
  {
    SetHintContents(aHintContents);
    MediaStreamGraph* gm = MediaStreamGraph::GetInstance();
    mStream = gm->CreateTrackUnionStream(this);
  }

  
  
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
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DOMLocalMediaStream, DOMMediaStream)
  NS_DECL_NSIDOMLOCALMEDIASTREAM

  NS_FORWARD_NSIDOMMEDIASTREAM(DOMMediaStream::)

  


  static already_AddRefed<DOMLocalMediaStream> CreateSourceStream(uint32_t aHintContents);

  


  static already_AddRefed<DOMLocalMediaStream> CreateTrackUnionStream(uint32_t aHintContents = 0);
};

}

#endif 
