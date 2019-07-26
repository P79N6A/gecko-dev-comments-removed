



#include "DOMCameraManager.h"

#include "mozilla/ErrorResult.h"

using namespace mozilla;


nsresult
nsDOMCameraManager::GetNumberOfCameras(int32_t& aDeviceCount)
{
  return NS_ERROR_NOT_IMPLEMENTED;
};

nsresult
nsDOMCameraManager::GetCameraName(uint32_t aDeviceNum, nsCString& aDeviceName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

void
nsDOMCameraManager::GetListOfCameras(nsTArray<nsString>& aList, ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
}
