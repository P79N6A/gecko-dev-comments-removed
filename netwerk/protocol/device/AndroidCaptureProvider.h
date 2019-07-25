




#ifndef AndroidDeviceCaptureProvide_h_
#define AndroidDeviceCaptureProvide_h_

#include "nsDeviceCaptureProvider.h"
#include "nsIAsyncInputStream.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "mozilla/net/CameraStreamImpl.h"
#include "nsIEventTarget.h"
#include "nsDeque.h"
#include "mozilla/ReentrantMonitor.h"

class AndroidCaptureProvider : public nsDeviceCaptureProvider {
  public:
    AndroidCaptureProvider();
    ~AndroidCaptureProvider();

    NS_DECL_ISUPPORTS

    nsresult Init(nsACString& aContentType, nsCaptureParams* aParams, nsIInputStream** aStream);
    static AndroidCaptureProvider* sInstance;
};

class AndroidCameraInputStream : public nsIAsyncInputStream, mozilla::net::CameraStreamImpl::FrameCallback {
  public:
    AndroidCameraInputStream();
    ~AndroidCameraInputStream();

    NS_IMETHODIMP Init(nsACString& aContentType, nsCaptureParams* aParams);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIASYNCINPUTSTREAM

    void ReceiveFrame(char* frame, uint32_t length);

  protected:
    void NotifyListeners();
    void doClose();

    uint32_t mAvailable;
    nsCString mContentType;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mCamera;
    bool mHeaderSent;
    bool mClosed;
    nsDeque *mFrameQueue;
    uint32_t mFrameSize;
    mozilla::ReentrantMonitor mMonitor;

    nsCOMPtr<nsIInputStreamCallback> mCallback;
    nsCOMPtr<nsIEventTarget> mCallbackTarget;
};

already_AddRefed<AndroidCaptureProvider> GetAndroidCaptureProvider();

#endif
