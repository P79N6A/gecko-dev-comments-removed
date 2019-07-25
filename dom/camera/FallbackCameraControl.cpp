



#include "nsDOMClassInfoID.h"
#include "DOMCameraManager.h"
#include "CameraControl.h"

using namespace mozilla;





class nsFallbackCameraControl : public nsCameraControl
{
public:
  nsFallbackCameraControl(PRUint32 aCameraId, nsIThread* aCameraThread);

  const char* GetParameter(const char* aKey);
  const char* GetParameterConstChar(PRUint32 aKey);
  double GetParameterDouble(PRUint32 aKey);
  void GetParameter(PRUint32 aKey, nsTArray<dom::CameraRegion>& aRegions);
  void SetParameter(const char* aKey, const char* aValue);
  void SetParameter(PRUint32 aKey, const char* aValue);
  void SetParameter(PRUint32 aKey, double aValue);
  void SetParameter(PRUint32 aKey, const nsTArray<dom::CameraRegion>& aRegions);
  void PushParameters();

protected:
  ~nsFallbackCameraControl();

  nsresult GetPreviewStreamImpl(GetPreviewStreamTask* aGetPreviewStream);
  nsresult AutoFocusImpl(AutoFocusTask* aAutoFocus);
  nsresult TakePictureImpl(TakePictureTask* aTakePicture);
  nsresult StartRecordingImpl(StartRecordingTask* aStartRecording);
  nsresult StopRecordingImpl(StopRecordingTask* aStopRecording);
  nsresult PushParametersImpl(PushParametersTask* aPushParameters);
  nsresult PullParametersImpl(PullParametersTask* aPullParameters);

private:
  nsFallbackCameraControl(const nsFallbackCameraControl&) MOZ_DELETE;
  nsFallbackCameraControl& operator=(const nsFallbackCameraControl&) MOZ_DELETE;
};







nsFallbackCameraControl::nsFallbackCameraControl(PRUint32 aCameraId, nsIThread* aCameraThread)
  : nsCameraControl(aCameraId, aCameraThread)
{ }

nsFallbackCameraControl::~nsFallbackCameraControl()
{ }

const char*
nsFallbackCameraControl::GetParameter(const char* aKey)
{
  return nullptr;
}

const char*
nsFallbackCameraControl::GetParameterConstChar(PRUint32 aKey)
{
  return nullptr;
}

double
nsFallbackCameraControl::GetParameterDouble(PRUint32 aKey)
{
  return NAN;
}

void
nsFallbackCameraControl::GetParameter(PRUint32 aKey, nsTArray<dom::CameraRegion>& aRegions)
{
}

void
nsFallbackCameraControl::SetParameter(const char* aKey, const char* aValue)
{
}

void
nsFallbackCameraControl::SetParameter(PRUint32 aKey, const char* aValue)
{
}

void
nsFallbackCameraControl::SetParameter(PRUint32 aKey, double aValue)
{
}

void
nsFallbackCameraControl::SetParameter(PRUint32 aKey, const nsTArray<dom::CameraRegion>& aRegions)
{
}

void
nsFallbackCameraControl::PushParameters()
{
}

nsresult
nsFallbackCameraControl::GetPreviewStreamImpl(GetPreviewStreamTask* aGetPreviewStream)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::AutoFocusImpl(AutoFocusTask* aAutoFocus)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::TakePictureImpl(TakePictureTask* aTakePicture)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::StartRecordingImpl(StartRecordingTask* aStartRecording)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::StopRecordingImpl(StopRecordingTask* aStopRecording)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::PushParametersImpl(PushParametersTask* aPushParameters)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsFallbackCameraControl::PullParametersImpl(PullParametersTask* aPullParameters)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
