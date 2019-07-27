





#ifndef CAPTURETASK_H
#define CAPTURETASK_H

#include "DOMMediaStream.h"
#include "MediaStreamGraph.h"

namespace mozilla {

namespace dom {
class Blob;
class ImageCapture;
} 











class CaptureTask : public MediaStreamListener,
                    public DOMMediaStream::PrincipalChangeObserver
{
public:
  
  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        StreamTime aTrackOffset,
                                        uint32_t aTrackEvents,
                                        const MediaSegment& aQueuedMedia) override;

  virtual void NotifyEvent(MediaStreamGraph* aGraph,
                           MediaStreamGraphEvent aEvent) override;

  
  virtual void PrincipalChanged(DOMMediaStream* aMediaStream) override;

  

  
  
  
  
  
  nsresult TaskComplete(already_AddRefed<dom::Blob> aBlob, nsresult aRv);

  
  
  void AttachStream();

  
  
  void DetachStream();

  
  CaptureTask(dom::ImageCapture* aImageCapture, TrackID aTrackID)
    : mImageCapture(aImageCapture)
    , mTrackID(aTrackID)
    , mImageGrabbedOrTrackEnd(false)
    , mPrincipalChanged(false) {}

protected:
  virtual ~CaptureTask() {}

  
  
  void PostTrackEndEvent();

  
  
  
  nsRefPtr<dom::ImageCapture> mImageCapture;

  TrackID mTrackID;

  
  
  bool mImageGrabbedOrTrackEnd;

  
  
  bool mPrincipalChanged;
};

} 

#endif 
