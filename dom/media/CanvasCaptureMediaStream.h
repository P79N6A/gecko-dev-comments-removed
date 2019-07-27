




#ifndef mozilla_dom_CanvasCaptureMediaStream_h_
#define mozilla_dom_CanvasCaptureMediaStream_h_

namespace mozilla {
class DOMMediaStream;

namespace dom {
class HTMLCanvasElement;

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
};

} 
} 

#endif 
