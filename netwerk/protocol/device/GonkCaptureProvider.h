















#ifndef GonkDeviceCaptureProvider_h_
#define GonkDeviceCaptureProvider_h_

#include "nsDeviceCaptureProvider.h"
#include "nsIAsyncInputStream.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsIEventTarget.h"
#include "nsDeque.h"
#include "mozilla/ReentrantMonitor.h"

#include "binder/IMemory.h"

using namespace android;

class CameraHardwareInterface;

class GonkCaptureProvider : public nsDeviceCaptureProvider {
  public:
    GonkCaptureProvider();
    ~GonkCaptureProvider();

    NS_DECL_ISUPPORTS

    nsresult Init(nsACString& aContentType, nsCaptureParams* aParams, nsIInputStream** aStream);
    static GonkCaptureProvider* sInstance;
};

class GonkCameraInputStream : public nsIAsyncInputStream {
  public:
    GonkCameraInputStream();
    ~GonkCameraInputStream();

    NS_IMETHODIMP Init(nsACString& aContentType, nsCaptureParams* aParams);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIASYNCINPUTSTREAM

    void ReceiveFrame(char* frame, uint32_t length);

    static void  DataCallback(int32_t aMsgType, const sp<IMemory>& aDataPtr, void *aUser);
    static uint32_t getNumberOfCameras();

  protected:
    void NotifyListeners();
    void doClose();

  private:
    uint32_t mAvailable;
    nsCString mContentType;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mFps;
    uint32_t mCamera;
    bool mHeaderSent;
    bool mClosed;
    bool mIs420p;
    nsDeque mFrameQueue;
    uint32_t mFrameSize;
    mozilla::ReentrantMonitor mMonitor;
    nsCOMPtr<nsIInputStreamCallback> mCallback;
    nsCOMPtr<nsIEventTarget> mCallbackTarget;
    CameraHardwareInterface* mHardware;
};

already_AddRefed<GonkCaptureProvider> GetGonkCaptureProvider();

#endif
