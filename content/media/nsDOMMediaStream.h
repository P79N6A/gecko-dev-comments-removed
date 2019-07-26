




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




class nsDOMMediaStream : public nsIDOMMediaStream
{
  friend class nsDOMLocalMediaStream;
  typedef mozilla::MediaStream MediaStream;

public:
  nsDOMMediaStream() : mStream(nullptr), mHintContents(0) {}
  virtual ~nsDOMMediaStream();

  NS_DECL_CYCLE_COLLECTION_CLASS(nsDOMMediaStream)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_NSIDOMMEDIASTREAM

  MediaStream* GetStream() { return mStream; }
  bool IsFinished() { return !mStream || mStream->IsFinished(); }
  



  nsIPrincipal* GetPrincipal() { return mPrincipal; }

  





  bool CombineWithPrincipal(nsIPrincipal* aPrincipal);

  


  static already_AddRefed<nsDOMMediaStream> CreateSourceStream(uint32_t aHintContents);

  
  
  enum {
    HINT_CONTENTS_AUDIO = 0x00000001U,
    HINT_CONTENTS_VIDEO = 0x00000002U
  };
  uint32_t GetHintContents() const { return mHintContents; }
  void SetHintContents(uint32_t aHintContents) { mHintContents = aHintContents; }

  


  static already_AddRefed<nsDOMMediaStream> CreateTrackUnionStream(uint32_t aHintContents = 0);

protected:
  
  
  MediaStream* mStream;
  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  
  uint32_t mHintContents;
};

class nsDOMLocalMediaStream : public nsDOMMediaStream,
                              public nsIDOMLocalMediaStream
{
public:
  nsDOMLocalMediaStream() {}
  virtual ~nsDOMLocalMediaStream() {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMLocalMediaStream, nsDOMMediaStream)
  NS_DECL_NSIDOMLOCALMEDIASTREAM

  NS_FORWARD_NSIDOMMEDIASTREAM(nsDOMMediaStream::)

  


  static already_AddRefed<nsDOMLocalMediaStream> CreateSourceStream(uint32_t aHintContents);

  


  static already_AddRefed<nsDOMLocalMediaStream> CreateTrackUnionStream(uint32_t aHintContents = 0);
};

#endif 
