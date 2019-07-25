



#include "nsDOMClassInfoID.h"
#include "DOMCameraManager.h"
#include "CameraControl.h"

using namespace mozilla;





class nsFallbackCameraControl : public nsCameraControl
{
public:
  nsFallbackCameraControl(uint32_t aCameraId, nsIThread* aCameraThread);

  const char* GetParameter(const char* aKey);
  const char* GetParameterConstChar(uint32_t aKey);
  double GetParameterDouble(uint32_t aKey);
  void GetParameter(uint32_t aKey, nsTArray<dom::CameraRegion>& aRegions);
  void SetParameter(const char* aKey, const char* aValue);
  void SetParameter(uint32_t aKey, const char* aValue);
  void SetParameter(uint32_t aKey, double aValue);
  void SetParameter(uint32_t aKey, const nsTArray<dom::CameraRegion>& aRegions);
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







nsFallbackCameraControl::nsFallbackCameraControl(uint32_t aCameraId, nsIThread* aCameraThread)
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
nsFallbackCameraControl::GetParameterConstChar(uint32_t aKey)
{
  return nullptr;
}

double
nsFallbackCameraControl::GetParameterDouble(uint32_t aKey)
{
  return NAN;
}

void
nsFallbackCameraControl::GetParameter(uint32_t aKey, nsTArray<dom::CameraRegion>& aRegions)
{
}

void
nsFallbackCameraControl::SetParameter(const char* aKey, const char* aValue)
{
}

void
nsFallbackCameraControl::SetParameter(uint32_t aKey, const char* aValue)
{
}

void
nsFallbackCameraControl::SetParameter(uint32_t aKey, double aValue)
{
}

void
nsFallbackCameraControl::SetParameter(uint32_t aKey, const nsTArray<dom::CameraRegion>& aRegions)
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
