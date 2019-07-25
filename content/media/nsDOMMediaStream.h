




#ifndef NSDOMMEDIASTREAM_H_
#define NSDOMMEDIASTREAM_H_

#include "nsIDOMMediaStream.h"
#include "MediaStreamGraph.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIPrincipal.h"




#ifdef GetCurrentTime
#undef GetCurrentTime
#endif




class nsDOMMediaStream : public nsIDOMMediaStream
{
  typedef mozilla::MediaStream MediaStream;

public:
  nsDOMMediaStream() : mStream(nullptr) {}
  virtual ~nsDOMMediaStream();

  NS_DECL_CYCLE_COLLECTION_CLASS(nsDOMMediaStream)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_NSIDOMMEDIASTREAM

  MediaStream* GetStream() { return mStream; }
  bool IsFinished() { return !mStream || mStream->IsFinished(); }
  



  nsIPrincipal* GetPrincipal() { return mPrincipal; }

  





  bool CombineWithPrincipal(nsIPrincipal* aPrincipal);

  


  static already_AddRefed<nsDOMMediaStream> CreateInputStream();

  


  static already_AddRefed<nsDOMMediaStream> CreateTrackUnionStream();

protected:
  
  
  MediaStream* mStream;
  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;
};

#endif 
