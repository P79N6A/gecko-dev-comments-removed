




#include "MediaEngineTabVideoSource.h"

#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "nsGlobalWindow.h"
#include "nsDOMWindowUtils.h"
#include "nsIDOMClientRect.h"
#include "nsIDocShell.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "gfxContext.h"
#include "gfx2DGlue.h"
#include "ImageContainer.h"
#include "Layers.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDOMDocument.h"
#include "nsITabSource.h"
#include "VideoUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIPrefService.h"
#include "MediaTrackConstraints.h"

namespace mozilla {

using namespace mozilla::gfx;
using dom::ConstrainLongRange;

NS_IMPL_ISUPPORTS(MediaEngineTabVideoSource, nsIDOMEventListener, nsITimerCallback)

MediaEngineTabVideoSource::MediaEngineTabVideoSource()
: mMonitor("MediaEngineTabVideoSource"), mTabSource(nullptr)
{
}

nsresult
MediaEngineTabVideoSource::StartRunnable::Run()
{
  mVideoSource->Draw();
  mVideoSource->mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  mVideoSource->mTimer->InitWithCallback(mVideoSource, mVideoSource->mTimePerFrame, nsITimer:: TYPE_REPEATING_SLACK);
  if (mVideoSource->mTabSource) {
    mVideoSource->mTabSource->NotifyStreamStart(mVideoSource->mWindow);
  }
  return NS_OK;
}

nsresult
MediaEngineTabVideoSource::StopRunnable::Run()
{
  nsCOMPtr<nsPIDOMWindow> privateDOMWindow = do_QueryInterface(mVideoSource->mWindow);

  if (mVideoSource->mTimer) {
    mVideoSource->mTimer->Cancel();
    mVideoSource->mTimer = nullptr;
  }
  if (mVideoSource->mTabSource) {
    mVideoSource->mTabSource->NotifyStreamStop(mVideoSource->mWindow);
  }
  return NS_OK;
}

NS_IMETHODIMP
MediaEngineTabVideoSource::HandleEvent(nsIDOMEvent *event) {
  Draw();
  return NS_OK;
}

NS_IMETHODIMP
MediaEngineTabVideoSource::Notify(nsITimer*) {
  Draw();
  return NS_OK;
}
#define LOGTAG "TabVideo"

nsresult
MediaEngineTabVideoSource::InitRunnable::Run()
{
  mVideoSource->mData = (unsigned char*)malloc(mVideoSource->mBufW * mVideoSource->mBufH * 4);
  if (mVideoSource->mWindowId != -1) {
    nsCOMPtr<nsPIDOMWindow> window  = nsGlobalWindow::GetOuterWindowWithId(mVideoSource->mWindowId);
    if (window) {
      mVideoSource->mWindow = window;
    }
  }
  if (!mVideoSource->mWindow) {
    nsresult rv;
    mVideoSource->mTabSource = do_GetService(NS_TABSOURCESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMWindow> win;
    rv = mVideoSource->mTabSource->GetTabToStream(getter_AddRefs(win));
    NS_ENSURE_SUCCESS(rv, rv);
    if (!win)
      return NS_OK;

    mVideoSource->mWindow = win;
  }
  nsCOMPtr<nsIRunnable> start(new StartRunnable(mVideoSource));
  start->Run();
  return NS_OK;
}

void
MediaEngineTabVideoSource::GetName(nsAString_internal& aName)
{
  aName.AssignLiteral(MOZ_UTF16("&getUserMedia.videoSource.tabShare;"));
}

void
MediaEngineTabVideoSource::GetUUID(nsAString_internal& aUuid)
{
  aUuid.AssignLiteral(MOZ_UTF16("uuid"));
}

nsresult
MediaEngineTabVideoSource::Allocate(const VideoTrackConstraintsN& aConstraints,
                                    const MediaEnginePrefs& aPrefs)
{

  ConstrainLongRange cWidth(aConstraints.mRequired.mWidth);
  ConstrainLongRange cHeight(aConstraints.mRequired.mHeight);

  mWindowId = aConstraints.mBrowserWindow.WasPassed() ? aConstraints.mBrowserWindow.Value() : -1;
  bool haveScrollWithPage = aConstraints.mScrollWithPage.WasPassed();
  mScrollWithPage =  haveScrollWithPage ? aConstraints.mScrollWithPage.Value() : true;

  if (aConstraints.mAdvanced.WasPassed()) {
    const auto& advanced = aConstraints.mAdvanced.Value();
    for (uint32_t i = 0; i < advanced.Length(); i++) {
      if (cWidth.mMax >= advanced[i].mWidth.mMin && cWidth.mMin <= advanced[i].mWidth.mMax &&
         cHeight.mMax >= advanced[i].mHeight.mMin && cHeight.mMin <= advanced[i].mHeight.mMax) {
        cWidth.mMin = std::max(cWidth.mMin, advanced[i].mWidth.mMin);
        cHeight.mMin = std::max(cHeight.mMin, advanced[i].mHeight.mMin);
      }

      if (mWindowId == -1 && advanced[i].mBrowserWindow.WasPassed()) {
        mWindowId = advanced[i].mBrowserWindow.Value();
      }

      if (!haveScrollWithPage && advanced[i].mScrollWithPage.WasPassed()) {
        mScrollWithPage = advanced[i].mScrollWithPage.Value();
        haveScrollWithPage = true;
      }
    }
  }

  mBufW = aPrefs.GetWidth(false);
  mBufH = aPrefs.GetHeight(false);

  if (cWidth.mMin > mBufW) {
    mBufW = cWidth.mMin;
  } else if (cWidth.mMax < mBufW) {
    mBufW = cWidth.mMax;
  }

  if (cHeight.mMin > mBufH) {
    mBufH = cHeight.mMin;
  } else if (cHeight.mMax < mBufH) {
    mBufH = cHeight.mMax;
  }

  mTimePerFrame = aPrefs.mFPS ? 1000 / aPrefs.mFPS : aPrefs.mFPS;
  return NS_OK;
}

nsresult
MediaEngineTabVideoSource::Deallocate()
{
  return NS_OK;
}

nsresult
MediaEngineTabVideoSource::Start(SourceMediaStream* aStream, TrackID aID)
{
  nsCOMPtr<nsIRunnable> runnable;
  if (!mWindow)
    runnable = new InitRunnable(this);
  else
    runnable = new StartRunnable(this);
  NS_DispatchToMainThread(runnable);
  aStream->AddTrack(aID, aStream->GraphRate(), 0, new VideoSegment());
  aStream->AdvanceKnownTracksTime(STREAM_TIME_MAX);

  return NS_OK;
}

void
MediaEngineTabVideoSource::NotifyPull(MediaStreamGraph*,
                                      SourceMediaStream* aSource,
                                      TrackID aID, StreamTime aDesiredTime,
                                      TrackTicks& aLastEndTime)
{
  VideoSegment segment;
  MonitorAutoLock mon(mMonitor);

  
  nsRefPtr<layers::CairoImage> image = mImage;
  TrackTicks target = aSource->TimeToTicksRoundUp(
      aSource->GraphRate(), aDesiredTime);
  TrackTicks delta = target - aLastEndTime;
  if (delta > 0) {
    
    gfx::IntSize size = image ? image->GetSize() : IntSize(0, 0);
    segment.AppendFrame(image.forget().downcast<layers::Image>(), delta, size);
    
    
    if (aSource->AppendToTrack(aID, &(segment))) {
      aLastEndTime = target;
    }
  }
}

void
MediaEngineTabVideoSource::Draw() {

  IntSize size(mBufW, mBufH);

  nsresult rv;
  float scale = 1.0;

  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(mWindow);

  if (!win) {
    return;
  }

  
  nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(win);
  if (!utils) {
    return;
  }

  nsCOMPtr<nsIDOMClientRect> rect;
  rv = utils->GetRootBounds(getter_AddRefs(rect));
  NS_ENSURE_SUCCESS_VOID(rv);
  if (!rect) {
    return;
  }

  float left, top, width, height;
  rect->GetLeft(&left);
  rect->GetTop(&top);
  rect->GetWidth(&width);
  rect->GetHeight(&height);

  if (mScrollWithPage) {
    nsPoint point;
    utils->GetScrollXY(false, &point.x, &point.y);
    left += point.x;
    top += point.y;
  }

  if (width == 0 || height == 0) {
    return;
  }

  int32_t srcX = left;
  int32_t srcY = top;
  int32_t srcW;
  int32_t srcH;

  float aspectRatio = ((float) size.width) / size.height;
  if (width / aspectRatio < height) {
    srcW = width;
    srcH = width / aspectRatio;
  } else {
    srcW = height * aspectRatio;
    srcH = height;
  }

  nsRefPtr<nsPresContext> presContext;
  nsIDocShell* docshell = win->GetDocShell();
  if (docshell) {
    docshell->GetPresContext(getter_AddRefs(presContext));
  }
  if (!presContext) {
    return;
  }
  nscolor bgColor = NS_RGB(255, 255, 255);
  nsCOMPtr<nsIPresShell> presShell = presContext->PresShell();
  uint32_t renderDocFlags = (nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING |
                             nsIPresShell::RENDER_DOCUMENT_RELATIVE);
  nsRect r(nsPresContext::CSSPixelsToAppUnits(srcX / scale),
           nsPresContext::CSSPixelsToAppUnits(srcY / scale),
           nsPresContext::CSSPixelsToAppUnits(srcW / scale),
           nsPresContext::CSSPixelsToAppUnits(srcH / scale));

  gfxImageFormat format = gfxImageFormat::RGB24;
  uint32_t stride = gfxASurface::FormatStrideForWidth(format, size.width);

  nsRefPtr<layers::ImageContainer> container = layers::LayerManager::CreateImageContainer();
  RefPtr<DrawTarget> dt =
    Factory::CreateDrawTargetForData(BackendType::CAIRO,
                                     mData.rwget(),
                                     size,
                                     stride,
                                     SurfaceFormat::B8G8R8X8);
  if (!dt) {
    return;
  }
  nsRefPtr<gfxContext> context = new gfxContext(dt);
  context->SetMatrix(
    context->CurrentMatrix().Scale(scale * size.width / srcW,
                                   scale * size.height / srcH));
  rv = presShell->RenderDocument(r, renderDocFlags, bgColor, context);

  NS_ENSURE_SUCCESS_VOID(rv);

  RefPtr<SourceSurface> surface = dt->Snapshot();
  if (!surface) {
    return;
  }

  layers::CairoImage::Data cairoData;
  cairoData.mSize = size;
  cairoData.mSourceSurface = surface;

  nsRefPtr<layers::CairoImage> image = new layers::CairoImage();

  image->SetData(cairoData);

  MonitorAutoLock mon(mMonitor);
  mImage = image;
}

nsresult
MediaEngineTabVideoSource::Stop(mozilla::SourceMediaStream*, mozilla::TrackID)
{
  if (!mWindow)
    return NS_OK;

  NS_DispatchToMainThread(new StopRunnable(this));
  return NS_OK;
}

nsresult
MediaEngineTabVideoSource::Config(bool, uint32_t, bool, uint32_t, bool, uint32_t, int32_t)
{
  return NS_OK;
}

bool
MediaEngineTabVideoSource::IsFake()
{
  return false;
}

}
