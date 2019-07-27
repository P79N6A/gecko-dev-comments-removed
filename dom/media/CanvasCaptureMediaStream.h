




#ifndef mozilla_dom_CanvasCaptureMediaStream_h_
#define mozilla_dom_CanvasCaptureMediaStream_h_

#include "DOMMediaStream.h"
#include "StreamBuffer.h"

namespace mozilla {
class DOMMediaStream;
class MediaStreamListener;
class SourceMediaStream;

namespace layers {
class Image;
} 

namespace dom {
class CanvasCaptureMediaStream;
class HTMLCanvasElement;

class OutputStreamDriver
{
public:
  OutputStreamDriver(CanvasCaptureMediaStream* aDOMStream,
                     const TrackID& aTrackId);

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(OutputStreamDriver);

  nsresult Start();

  virtual void ForgetDOMStream();

  virtual void RequestFrame() { }

  CanvasCaptureMediaStream* DOMStream() const { return mDOMStream; }

protected:
  virtual ~OutputStreamDriver();
  class StreamListener;

  


  void AppendToTrack(StreamTime aDuration);
  void NotifyPull(StreamTime aDesiredTime);

  



  void SetImage(layers::Image* aImage);

  


  virtual void StartInternal() = 0;

private:
  
  
  
  CanvasCaptureMediaStream* mDOMStream;
  nsRefPtr<SourceMediaStream> mSourceStream;
  bool mStarted;
  nsRefPtr<StreamListener> mStreamListener;
  const TrackID mTrackId;

  
  Mutex mMutex;
  nsRefPtr<layers::Image> mImage;
};

class CanvasCaptureMediaStream: public DOMMediaStream
{
public:
  explicit CanvasCaptureMediaStream(HTMLCanvasElement* aCanvas);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(CanvasCaptureMediaStream, DOMMediaStream)

  nsresult Init(const dom::Optional<double>& aFPS, const TrackID& aTrackId);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  HTMLCanvasElement* Canvas() const { return mCanvas; }
  void RequestFrame();

  


  static already_AddRefed<CanvasCaptureMediaStream>
  CreateSourceStream(nsIDOMWindow* aWindow,
                     HTMLCanvasElement* aCanvas);

protected:
  ~CanvasCaptureMediaStream();

private:
  nsRefPtr<HTMLCanvasElement> mCanvas;
  nsRefPtr<OutputStreamDriver> mOutputStreamDriver;
};

} 
} 

#endif 
