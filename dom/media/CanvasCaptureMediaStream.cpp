




#include "CanvasCaptureMediaStream.h"
#include "DOMMediaStream.h"
#include "gfxPlatform.h"
#include "ImageContainer.h"
#include "MediaStreamGraph.h"
#include "mozilla/Mutex.h"
#include "mozilla/dom/CanvasCaptureMediaStreamBinding.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "nsIAppShell.h"
#include "nsWidgetsCID.h"

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

using namespace mozilla::layers;
using namespace mozilla::gfx;

namespace mozilla {
namespace dom {

class OutputStreamDriver::StreamListener : public MediaStreamListener
{
public:
  explicit StreamListener(OutputStreamDriver* aDriver,
                          SourceMediaStream* aSourceStream)
    : mSourceStream(aSourceStream)
    , mMutex("CanvasCaptureMediaStream::OSD::StreamListener")
    , mDriver(aDriver)
  {
    MOZ_ASSERT(mDriver);
    MOZ_ASSERT(mSourceStream);
  }

  void Forget() {
    MOZ_ASSERT(NS_IsMainThread());

    MutexAutoLock lock(mMutex);
    mDriver = nullptr;
  }

  virtual void NotifyPull(MediaStreamGraph* aGraph, StreamTime aDesiredTime) override
  {
    

    MutexAutoLock lock(mMutex);
    if (mDriver) {
      mDriver->NotifyPull(aDesiredTime);
    } else {
      
      mSourceStream->EndAllTrackAndFinish();
    }
  }

protected:
  ~StreamListener() { }

private:
  nsRefPtr<SourceMediaStream> mSourceStream;

  
  Mutex mMutex;
  
  
  OutputStreamDriver* mDriver;
};

OutputStreamDriver::OutputStreamDriver(CanvasCaptureMediaStream* aDOMStream,
                                       const TrackID& aTrackId)
  : mDOMStream(aDOMStream)
  , mSourceStream(nullptr)
  , mStarted(false)
  , mStreamListener(nullptr)
  , mTrackId(aTrackId)
  , mMutex("CanvasCaptureMediaStream::OutputStreamDriver")
  , mImage(nullptr)
{
  MOZ_ASSERT(mDOMStream);
}

OutputStreamDriver::~OutputStreamDriver()
{
  if (mStreamListener) {
    
    
    mStreamListener->Forget();
  }
}

nsresult
OutputStreamDriver::Start()
{
  if (mStarted) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  MOZ_ASSERT(mDOMStream);

  mDOMStream->CreateDOMTrack(mTrackId, MediaSegment::VIDEO);

  mSourceStream = mDOMStream->GetStream()->AsSourceStream();
  MOZ_ASSERT(mSourceStream);

  mStreamListener = new StreamListener(this, mSourceStream);
  mSourceStream->AddListener(mStreamListener);
  mSourceStream->AddTrack(mTrackId, 0, new VideoSegment());
  mSourceStream->AdvanceKnownTracksTime(STREAM_TIME_MAX);
  mSourceStream->SetPullEnabled(true);

  
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &OutputStreamDriver::StartInternal);
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  appShell->RunInStableState(runnable);

  mStarted = true;
  return NS_OK;
}

void
OutputStreamDriver::ForgetDOMStream()
{
  if (mStreamListener) {
    mStreamListener->Forget();
  }
  mDOMStream = nullptr;
}

void
OutputStreamDriver::AppendToTrack(StreamTime aDuration)
{
  MOZ_ASSERT(mSourceStream);

  MutexAutoLock lock(mMutex);

  nsRefPtr<Image> image = mImage;
  IntSize size = image ? image->GetSize() : IntSize(0, 0);
  VideoSegment segment;
  segment.AppendFrame(image.forget(), aDuration, size);

  mSourceStream->AppendToTrack(mTrackId, &segment);
}

void
OutputStreamDriver::NotifyPull(StreamTime aDesiredTime)
{
  StreamTime delta = aDesiredTime - mSourceStream->GetEndOfAppendedData(mTrackId);
  if (delta > 0) {
    
    AppendToTrack(delta);
  }
}

void
OutputStreamDriver::SetImage(Image* aImage)
{
  MutexAutoLock lock(mMutex);
  mImage = aImage;
}



class TimerDriver : public OutputStreamDriver
                  , public nsITimerCallback
{
public:
  explicit TimerDriver(CanvasCaptureMediaStream* aDOMStream,
                       const double& aFPS,
                       const TrackID& aTrackId)
    : OutputStreamDriver(aDOMStream, aTrackId)
    , mFPS(aFPS)
    , mTimer(nullptr)
  {
  }

  void ForgetDOMStream() override
  {
    if (mTimer) {
      mTimer->Cancel();
      mTimer = nullptr;
    }
    OutputStreamDriver::ForgetDOMStream();
  }

  nsresult
  TakeSnapshot()
  {
    
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(DOMStream());

    if (!DOMStream()->Canvas()) {
      
      return NS_ERROR_NOT_AVAILABLE;
    }
    MOZ_ASSERT(DOMStream()->Canvas());

    if (DOMStream()->Canvas()->IsWriteOnly()) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }

    
    RefPtr<SourceSurface> snapshot = DOMStream()->Canvas()->GetSurfaceSnapshot(nullptr);
    if (!snapshot) {
      return NS_ERROR_FAILURE;
    }

    RefPtr<SourceSurface> opt = gfxPlatform::GetPlatform()
      ->ScreenReferenceDrawTarget()->OptimizeSourceSurface(snapshot);
    if (!opt) {
      return NS_ERROR_FAILURE;
    }

    CairoImage::Data imageData;
    imageData.mSize = opt->GetSize();
    imageData.mSourceSurface = opt;

    RefPtr<CairoImage> image = new layers::CairoImage();
    image->SetData(imageData);

    SetImage(image);
    return NS_OK;
  }

  NS_IMETHODIMP
  Notify(nsITimer* aTimer) override
  {
    nsresult rv = TakeSnapshot();
    if (NS_FAILED(rv)) {
      aTimer->Cancel();
    }
    return rv;
  }

  virtual void RequestFrame() override
  {
    TakeSnapshot();
  }

  NS_DECL_ISUPPORTS_INHERITED

protected:
  virtual ~TimerDriver() {}

  virtual void StartInternal() override
  {
    
    DebugOnly<nsresult> rv = TakeSnapshot();
    MOZ_ASSERT(NS_SUCCEEDED(rv));

    if (mFPS == 0.0) {
      return;
    }

    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
    if (!mTimer) {
      return;
    }
    mTimer->InitWithCallback(this, int(1000 / mFPS), nsITimer::TYPE_REPEATING_SLACK);
  }

private:
  const double mFPS;
  nsCOMPtr<nsITimer> mTimer;
};

NS_IMPL_ADDREF_INHERITED(TimerDriver, OutputStreamDriver)
NS_IMPL_RELEASE_INHERITED(TimerDriver, OutputStreamDriver)
NS_IMPL_QUERY_INTERFACE(TimerDriver, nsITimerCallback)



NS_IMPL_CYCLE_COLLECTION_INHERITED(CanvasCaptureMediaStream, DOMMediaStream,
                                   mCanvas)

NS_IMPL_ADDREF_INHERITED(CanvasCaptureMediaStream, DOMMediaStream)
NS_IMPL_RELEASE_INHERITED(CanvasCaptureMediaStream, DOMMediaStream)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(CanvasCaptureMediaStream)
NS_INTERFACE_MAP_END_INHERITING(DOMMediaStream)

CanvasCaptureMediaStream::CanvasCaptureMediaStream(HTMLCanvasElement* aCanvas)
  : mCanvas(aCanvas)
  , mOutputStreamDriver(nullptr)
{
}

CanvasCaptureMediaStream::~CanvasCaptureMediaStream()
{
  if (mOutputStreamDriver) {
    mOutputStreamDriver->ForgetDOMStream();
  }
}

JSObject*
CanvasCaptureMediaStream::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return dom::CanvasCaptureMediaStreamBinding::Wrap(aCx, this, aGivenProto);
}

void
CanvasCaptureMediaStream::RequestFrame()
{
  if (mOutputStreamDriver) {
    mOutputStreamDriver->RequestFrame();
  }
}

nsresult
CanvasCaptureMediaStream::Init(const dom::Optional<double>& aFPS,
                               const TrackID& aTrackId)
{
  if (!aFPS.WasPassed()) {
    
    
    mOutputStreamDriver = new TimerDriver(this, 30.0, aTrackId);
  } else if (aFPS.Value() < 0) {
    return NS_ERROR_ILLEGAL_VALUE;
  } else {
    
    double fps = std::min(60.0, aFPS.Value());
    mOutputStreamDriver = new TimerDriver(this, fps, aTrackId);
  }
  return mOutputStreamDriver->Start();
}

already_AddRefed<CanvasCaptureMediaStream>
CanvasCaptureMediaStream::CreateSourceStream(nsIDOMWindow* aWindow,
                                             HTMLCanvasElement* aCanvas)
{
  nsRefPtr<CanvasCaptureMediaStream> stream = new CanvasCaptureMediaStream(aCanvas);
  stream->InitSourceStream(aWindow);
  return stream.forget();
}

} 
} 

