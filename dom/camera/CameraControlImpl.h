



#ifndef DOM_CAMERA_CAMERACONTROLIMPL_H
#define DOM_CAMERA_CAMERACONTROLIMPL_H

#include "nsCOMPtr.h"
#include "nsDOMFile.h"
#include "DictionaryHelpers.h"
#include "nsIDOMCameraManager.h"
#include "ICameraControl.h"
#include "CameraCommon.h"

namespace mozilla {

using namespace dom;

class GetPreviewStreamTask;
class StartPreviewTask;
class StopPreviewTask;
class AutoFocusTask;
class TakePictureTask;
class StartRecordingTask;
class StopRecordingTask;
class SetParameterTask;
class GetParameterTask;

class DOMCameraPreview;

class CameraControlImpl : public ICameraControl
{
  friend class GetPreviewStreamTask;
  friend class StartPreviewTask;
  friend class StopPreviewTask;
  friend class AutoFocusTask;
  friend class TakePictureTask;
  friend class StartRecordingTask;
  friend class StopRecordingTask;
  friend class SetParameterTask;
  friend class GetParameterTask;

public:
  CameraControlImpl(uint32_t aCameraId, nsIThread* aCameraThread)
    : mCameraId(aCameraId)
    , mCameraThread(aCameraThread)
    , mFileFormat()
    , mMaxMeteringAreas(0)
    , mMaxFocusAreas(0)
    , mDOMPreview(nullptr)
    , mAutoFocusOnSuccessCb(nullptr)
    , mAutoFocusOnErrorCb(nullptr)
    , mTakePictureOnSuccessCb(nullptr)
    , mTakePictureOnErrorCb(nullptr)
    , mStartRecordingOnSuccessCb(nullptr)
    , mStartRecordingOnErrorCb(nullptr)
    , mOnShutterCb(nullptr)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  nsresult GetPreviewStream(CameraSize aSize, nsICameraPreviewStreamCallback* onSuccess, nsICameraErrorCallback* onError);
  nsresult StartPreview(DOMCameraPreview* aDOMPreview);
  void StopPreview();
  nsresult AutoFocus(nsICameraAutoFocusCallback* onSuccess, nsICameraErrorCallback* onError);
  nsresult TakePicture(CameraSize aSize, int32_t aRotation, const nsAString& aFileFormat, CameraPosition aPosition, nsICameraTakePictureCallback* onSuccess, nsICameraErrorCallback* onError);
  nsresult StartRecording(CameraSize aSize, nsICameraStartRecordingCallback* onSuccess, nsICameraErrorCallback* onError);
  nsresult StopRecording();

  nsresult Set(uint32_t aKey, const nsAString& aValue);
  nsresult Get(uint32_t aKey, nsAString& aValue);
  nsresult Set(uint32_t aKey, double aValue);
  nsresult Get(uint32_t aKey, double* aValue);
  nsresult Set(JSContext* aCx, uint32_t aKey, const JS::Value& aValue, uint32_t aLimit);
  nsresult Get(JSContext* aCx, uint32_t aKey, JS::Value* aValue);

  nsresult SetFocusAreas(JSContext* aCx, const JS::Value& aValue)
  {
    return Set(aCx, CAMERA_PARAM_FOCUSAREAS, aValue, mMaxFocusAreas);
  }

  nsresult SetMeteringAreas(JSContext* aCx, const JS::Value& aValue)
  {
    return Set(aCx, CAMERA_PARAM_METERINGAREAS, aValue, mMaxMeteringAreas);
  }

  virtual const char* GetParameter(const char* aKey) = 0;
  virtual const char* GetParameterConstChar(uint32_t aKey) = 0;
  virtual double GetParameterDouble(uint32_t aKey) = 0;
  virtual void GetParameter(uint32_t aKey, nsTArray<CameraRegion>& aRegions) = 0;
  virtual void SetParameter(const char* aKey, const char* aValue) = 0;
  virtual void SetParameter(uint32_t aKey, const char* aValue) = 0;
  virtual void SetParameter(uint32_t aKey, double aValue) = 0;
  virtual void SetParameter(uint32_t aKey, const nsTArray<CameraRegion>& aRegions) = 0;
  virtual nsresult PushParameters() = 0;

  bool ReceiveFrame(void* aBuffer, ImageFormat aFormat, FrameBuilder aBuilder);

protected:
  virtual ~CameraControlImpl()
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  virtual nsresult GetPreviewStreamImpl(GetPreviewStreamTask* aGetPreviewStream) = 0;
  virtual nsresult StartPreviewImpl(StartPreviewTask* aStartPreview) = 0;
  virtual nsresult StopPreviewImpl(StopPreviewTask* aStopPreview) = 0;
  virtual nsresult AutoFocusImpl(AutoFocusTask* aAutoFocus) = 0;
  virtual nsresult TakePictureImpl(TakePictureTask* aTakePicture) = 0;
  virtual nsresult StartRecordingImpl(StartRecordingTask* aStartRecording) = 0;
  virtual nsresult StopRecordingImpl(StopRecordingTask* aStopRecording) = 0;
  virtual nsresult PushParametersImpl() = 0;
  virtual nsresult PullParametersImpl() = 0;

  uint32_t            mCameraId;
  nsCOMPtr<nsIThread> mCameraThread;
  nsString            mFileFormat;
  uint32_t            mMaxMeteringAreas;
  uint32_t            mMaxFocusAreas;

  







  DOMCameraPreview*   mDOMPreview;

  nsCOMPtr<nsICameraAutoFocusCallback>      mAutoFocusOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback>          mAutoFocusOnErrorCb;
  nsCOMPtr<nsICameraTakePictureCallback>    mTakePictureOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback>          mTakePictureOnErrorCb;
  nsCOMPtr<nsICameraStartRecordingCallback> mStartRecordingOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback>          mStartRecordingOnErrorCb;
  nsCOMPtr<nsICameraShutterCallback>        mOnShutterCb;

private:
  CameraControlImpl(const CameraControlImpl&) MOZ_DELETE;
  CameraControlImpl& operator=(const CameraControlImpl&) MOZ_DELETE;
};


class GetPreviewStreamResult : public nsRunnable
{
public:
  GetPreviewStreamResult(CameraControlImpl* aCameraControl, uint32_t aWidth, uint32_t aHeight, uint32_t aFramesPerSecond, nsICameraPreviewStreamCallback* onSuccess)
    : mCameraControl(aCameraControl)
    , mWidth(aWidth)
    , mHeight(aHeight)
    , mFramesPerSecond(aFramesPerSecond)
    , mOnSuccessCb(onSuccess)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  virtual ~GetPreviewStreamResult()
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  
  NS_IMETHOD Run();

protected:
  nsRefPtr<CameraControlImpl> mCameraControl;
  uint32_t mWidth;
  uint32_t mHeight;
  uint32_t mFramesPerSecond;
  nsCOMPtr<nsICameraPreviewStreamCallback> mOnSuccessCb;
};


class GetPreviewStreamTask : public nsRunnable
{
public:
  GetPreviewStreamTask(CameraControlImpl* aCameraControl, CameraSize aSize, nsICameraPreviewStreamCallback* onSuccess, nsICameraErrorCallback* onError)
    : mSize(aSize)
    , mCameraControl(aCameraControl)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  virtual ~GetPreviewStreamTask()
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  NS_IMETHOD Run()
  {
    nsresult rv = mCameraControl->GetPreviewStreamImpl(this);

    if (NS_FAILED(rv) && mOnErrorCb) {
      rv = NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE")));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return rv;
  }

  CameraSize mSize;
  nsRefPtr<CameraControlImpl> mCameraControl;
  nsCOMPtr<nsICameraPreviewStreamCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};


class AutoFocusResult : public nsRunnable
{
public:
  AutoFocusResult(bool aSuccess, nsICameraAutoFocusCallback* onSuccess)
    : mSuccess(aSuccess)
    , mOnSuccessCb(onSuccess)
  { }

  virtual ~AutoFocusResult() { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mSuccess);
    }
    return NS_OK;
  }

protected:
  bool mSuccess;
  nsCOMPtr<nsICameraAutoFocusCallback> mOnSuccessCb;
};


class AutoFocusTask : public nsRunnable
{
public:
  AutoFocusTask(CameraControlImpl* aCameraControl, nsICameraAutoFocusCallback* onSuccess, nsICameraErrorCallback* onError)
    : mCameraControl(aCameraControl)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  virtual ~AutoFocusTask()
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->AutoFocusImpl(this);
    DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv) && mOnErrorCb) {
      rv = NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE")));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return rv;
  }

  nsRefPtr<CameraControlImpl> mCameraControl;
  nsCOMPtr<nsICameraAutoFocusCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};


class TakePictureResult : public nsRunnable
{
public:
  TakePictureResult(nsIDOMBlob* aImage, nsICameraTakePictureCallback* onSuccess)
    : mImage(aImage)
    , mOnSuccessCb(onSuccess)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  virtual ~TakePictureResult()
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mImage);
    }
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
    return NS_OK;
  }

protected:
  nsCOMPtr<nsIDOMBlob> mImage;
  nsCOMPtr<nsICameraTakePictureCallback> mOnSuccessCb;
};


class TakePictureTask : public nsRunnable
{
public:
  TakePictureTask(CameraControlImpl* aCameraControl, CameraSize aSize, int32_t aRotation, const nsAString& aFileFormat, CameraPosition aPosition, nsICameraTakePictureCallback* onSuccess, nsICameraErrorCallback* onError)
    : mCameraControl(aCameraControl)
    , mSize(aSize)
    , mRotation(aRotation)
    , mFileFormat(aFileFormat)
    , mPosition(aPosition)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  virtual ~TakePictureTask()
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->TakePictureImpl(this);
    DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv) && mOnErrorCb) {
      rv = NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE")));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return rv;
  }

  nsRefPtr<CameraControlImpl> mCameraControl;
  CameraSize mSize;
  int32_t mRotation;
  nsString mFileFormat;
  CameraPosition mPosition;
  nsCOMPtr<nsICameraTakePictureCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};


class StartRecordingResult : public nsRunnable
{
public:
  StartRecordingResult(nsIDOMMediaStream* aStream, nsICameraStartRecordingCallback* onSuccess)
    : mStream(aStream)
    , mOnSuccessCb(onSuccess)
  { }

  virtual ~StartRecordingResult() { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mOnSuccessCb) {
      mOnSuccessCb->HandleEvent(mStream);
    }
    return NS_OK;
  }

protected:
  nsCOMPtr<nsIDOMMediaStream> mStream;
  nsCOMPtr<nsICameraStartRecordingCallback> mOnSuccessCb;
};


class StartRecordingTask : public nsRunnable
{
public:
  StartRecordingTask(CameraControlImpl* aCameraControl, CameraSize aSize, nsICameraStartRecordingCallback* onSuccess, nsICameraErrorCallback* onError)
    : mSize(aSize)
    , mCameraControl(aCameraControl)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  virtual ~StartRecordingTask()
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->StartRecordingImpl(this);
    DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);

    if (NS_FAILED(rv) && mOnErrorCb) {
      rv = NS_DispatchToMainThread(new CameraErrorResult(mOnErrorCb, NS_LITERAL_STRING("FAILURE")));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return rv;
  }

  CameraSize mSize;
  nsRefPtr<CameraControlImpl> mCameraControl;
  nsCOMPtr<nsICameraStartRecordingCallback> mOnSuccessCb;
  nsCOMPtr<nsICameraErrorCallback> mOnErrorCb;
};


class StopRecordingTask : public nsRunnable
{
public:
  StopRecordingTask(CameraControlImpl* aCameraControl)
    : mCameraControl(aCameraControl)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  virtual ~StopRecordingTask()
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->StopRecordingImpl(this);
    DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);

    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  nsRefPtr<CameraControlImpl> mCameraControl;
};


class StartPreviewTask : public nsRunnable
{
public:
  StartPreviewTask(CameraControlImpl* aCameraControl, DOMCameraPreview* aDOMPreview)
    : mCameraControl(aCameraControl)
    , mDOMPreview(aDOMPreview)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  virtual ~StartPreviewTask()
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
    nsresult rv = mCameraControl->StartPreviewImpl(this);
    DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);

    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  nsRefPtr<CameraControlImpl> mCameraControl;
  DOMCameraPreview* mDOMPreview; 
};


class StopPreviewTask : public nsRunnable
{
public:
  StopPreviewTask(CameraControlImpl* aCameraControl)
    : mCameraControl(aCameraControl)
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  virtual ~StopPreviewTask()
  {
    DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
  }

  NS_IMETHOD Run()
  {
    DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);
    mCameraControl->StopPreviewImpl(this);
    DOM_CAMERA_LOGT("%s:%d\n", __func__, __LINE__);

    return NS_OK;
  }

  nsRefPtr<CameraControlImpl> mCameraControl;
};

} 

#endif 
