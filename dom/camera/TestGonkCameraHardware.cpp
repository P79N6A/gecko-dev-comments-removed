















#include "TestGonkCameraHardware.h"

#include "mozilla/Preferences.h"
#include "nsThreadUtils.h"

using namespace android;
using namespace mozilla;

TestGonkCameraHardware::TestGonkCameraHardware(nsGonkCameraControl* aTarget,
                                               uint32_t aCameraId,
                                               const sp<Camera>& aCamera)
  : GonkCameraHardware(aTarget, aCameraId, aCamera)
{
  DOM_CAMERA_LOGA("+===== Created TestGonkCameraHardware =====+\n");
  DOM_CAMERA_LOGT("%s:%d : this=%p (aTarget=%p)\n",
    __func__, __LINE__, this, aTarget);
  MOZ_COUNT_CTOR(TestGonkCameraHardware);
}

TestGonkCameraHardware::~TestGonkCameraHardware()
{
  MOZ_COUNT_DTOR(TestGonkCameraHardware);
  DOM_CAMERA_LOGA("+===== Destroyed TestGonkCameraHardware =====+\n");
}

nsresult
TestGonkCameraHardware::Init()
{
  if (IsTestCase("init-failure")) {
    return NS_ERROR_FAILURE;
  }

  return GonkCameraHardware::Init();
}

const nsCString
TestGonkCameraHardware::TestCase()
{
  const nsCString test = Preferences::GetCString("camera.control.test.hardware");
  return test;
}

const nsCString
TestGonkCameraHardware::GetExtraParameters()
{
  

















  const nsCString parameters = Preferences::GetCString("camera.control.test.hardware.gonk.parameters");
  DOM_CAMERA_LOGA("TestGonkCameraHardware : extra-parameters '%s'\n",
    parameters.get());
  return parameters;
}

bool
TestGonkCameraHardware::IsTestCaseInternal(const char* aTest, const char* aFile, int aLine)
{
  if (TestCase().EqualsASCII(aTest)) {
    DOM_CAMERA_LOGA("TestGonkCameraHardware : test-case '%s' (%s:%d)\n",
      aTest, aFile, aLine);
    return true;
  }

  return false;
}

int
TestGonkCameraHardware::TestCaseError(int aDefaultError)
{
  
  return aDefaultError;
}

int
TestGonkCameraHardware::AutoFocus()
{
  class AutoFocusFailure : public nsRunnable
  {
  public:
    AutoFocusFailure(nsGonkCameraControl* aTarget)
      : mTarget(aTarget)
    { }

    NS_IMETHODIMP
    Run()
    {
      OnAutoFocusComplete(mTarget, false);
      return NS_OK;
    }

  protected:
    nsGonkCameraControl* mTarget;
  };

  if (IsTestCase("auto-focus-failure")) {
    return TestCaseError(UNKNOWN_ERROR);
  }
  if (IsTestCase("auto-focus-process-failure")) {
    nsresult rv = NS_DispatchToCurrentThread(new AutoFocusFailure(mTarget));
    if (NS_SUCCEEDED(rv)) {
      return OK;
    }
    DOM_CAMERA_LOGE("Failed to dispatch AutoFocusFailure runnable (0x%08x)\n", rv);
    return UNKNOWN_ERROR;
  }

  return GonkCameraHardware::AutoFocus();
}

int
TestGonkCameraHardware::TakePicture()
{
  class TakePictureFailure : public nsRunnable
  {
  public:
    TakePictureFailure(nsGonkCameraControl* aTarget)
      : mTarget(aTarget)
    { }

    NS_IMETHODIMP
    Run()
    {
      OnTakePictureError(mTarget);
      return NS_OK;
    }

  protected:
    nsGonkCameraControl* mTarget;
  };

  if (IsTestCase("take-picture-failure")) {
    return TestCaseError(UNKNOWN_ERROR);
  }
  if (IsTestCase("take-picture-process-failure")) {
    nsresult rv = NS_DispatchToCurrentThread(new TakePictureFailure(mTarget));
    if (NS_SUCCEEDED(rv)) {
      return OK;
    }
    DOM_CAMERA_LOGE("Failed to dispatch TakePictureFailure runnable (0x%08x)\n", rv);
    return UNKNOWN_ERROR;
  }

  return GonkCameraHardware::TakePicture();
}

int
TestGonkCameraHardware::StartPreview()
{
  if (IsTestCase("start-preview-failure")) {
    return TestCaseError(UNKNOWN_ERROR);
  }

  return GonkCameraHardware::StartPreview();
}

int
TestGonkCameraHardware::StartAutoFocusMoving(bool aIsMoving)
{
  class AutoFocusMoving : public nsRunnable
  {
  public:
    AutoFocusMoving(nsGonkCameraControl* aTarget, bool aIsMoving)
      : mTarget(aTarget)
      , mIsMoving(aIsMoving)
    { }

    NS_IMETHODIMP
    Run()
    {
      OnAutoFocusMoving(mTarget, mIsMoving);
      return NS_OK;
    }

  protected:
    nsGonkCameraControl* mTarget;
    bool mIsMoving;
  };

  nsresult rv = NS_DispatchToCurrentThread(new AutoFocusMoving(mTarget, aIsMoving));
  if (NS_SUCCEEDED(rv)) {
    return OK;
  }
  DOM_CAMERA_LOGE("Failed to dispatch AutoFocusMoving runnable (0x%08x)\n", rv);
  return UNKNOWN_ERROR;
}

int
TestGonkCameraHardware::PushParameters(const GonkCameraParameters& aParams)
{
  if (IsTestCase("push-parameters-failure")) {
    return TestCaseError(UNKNOWN_ERROR);
  }

  nsString focusMode;
  GonkCameraParameters& params = const_cast<GonkCameraParameters&>(aParams);
  params.Get(CAMERA_PARAM_FOCUSMODE, focusMode);
  if (focusMode.EqualsASCII("continuous-picture") ||
      focusMode.EqualsASCII("continuous-video"))
  {
    if (IsTestCase("autofocus-moving-true")) {
      return StartAutoFocusMoving(true);
    } else if (IsTestCase("autofocus-moving-false")) {
      return StartAutoFocusMoving(false);
    }
  }

  return GonkCameraHardware::PushParameters(aParams);
}

nsresult
TestGonkCameraHardware::PullParameters(GonkCameraParameters& aParams)
{
  if (IsTestCase("pull-parameters-failure")) {
    return static_cast<nsresult>(TestCaseError(UNKNOWN_ERROR));
  }

  String8 s = mCamera->getParameters();
  nsCString extra = GetExtraParameters();
  if (!extra.IsEmpty()) {
    s += ";";
    s += extra.get();
  }

  return aParams.Unflatten(s);
}

int
TestGonkCameraHardware::StartRecording()
{
  if (IsTestCase("start-recording-failure")) {
    return TestCaseError(UNKNOWN_ERROR);
  }

  return GonkCameraHardware::StartRecording();
}

int
TestGonkCameraHardware::StopRecording()
{
  if (IsTestCase("stop-recording-failure")) {
    return TestCaseError(UNKNOWN_ERROR);
  }

  return GonkCameraHardware::StopRecording();
}

int
TestGonkCameraHardware::SetListener(const sp<GonkCameraListener>& aListener)
{
  if (IsTestCase("set-listener-failure")) {
    return TestCaseError(UNKNOWN_ERROR);
  }

  return GonkCameraHardware::SetListener(aListener);
}

int
TestGonkCameraHardware::StoreMetaDataInBuffers(bool aEnabled)
{
  if (IsTestCase("store-metadata-in-buffers-failure")) {
    return TestCaseError(UNKNOWN_ERROR);
  }

  return GonkCameraHardware::StoreMetaDataInBuffers(aEnabled);
}
