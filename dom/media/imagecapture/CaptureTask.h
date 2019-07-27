





#ifndef CAPTURETASK_H
#define CAPTURETASK_H

#include "DOMMediaStream.h"
#include "MediaStreamGraph.h"

namespace mozilla {

namespace dom {
class File;
class ImageCapture;
}











class CaptureTask : public MediaStreamListener,
                    public DOMMediaStream::PrincipalChangeObserver
{
public:
  
  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        TrackTicks aTrackOffset,
                                        uint32_t aTrackEvents,
                                        const MediaSegment& aQueuedMedia) MOZ_OVERRIDE;

  virtual void NotifyEvent(MediaStreamGraph* aGraph,
                           MediaStreamGraphEvent aEvent) MOZ_OVERRIDE;

  
  virtual void PrincipalChanged(DOMMediaStream* aMediaStream) MOZ_OVERRIDE;

  

  
  
  
  
  
  nsresult TaskComplete(already_AddRefed<dom::File> aBlob, nsresult aRv);

  
  
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
