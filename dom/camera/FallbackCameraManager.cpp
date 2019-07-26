



#include "ICameraControl.h"

using namespace mozilla;


nsresult
ICameraControl::GetNumberOfCameras(int32_t& aDeviceCount)
{
  return NS_ERROR_NOT_IMPLEMENTED;
};

nsresult
ICameraControl::GetCameraName(uint32_t aDeviceNum, nsCString& aDeviceName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
ICameraControl::GetListOfCameras(nsTArray<nsString>& aList)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

already_AddRefed<ICameraControl>
ICameraControl::Create(uint32_t aCameraId)
{
  return nullptr;
}
