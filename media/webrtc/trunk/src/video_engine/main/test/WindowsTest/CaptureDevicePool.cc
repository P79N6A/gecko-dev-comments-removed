









#include "CaptureDevicePool.h"
#include "map_wrapper.h"
#include <string.h>
#include <assert.h>
#include "critical_section_wrapper.h"
#include "vie_file.h"

CaptureDevicePool::CaptureDevicePool(VideoEngine* videoEngine):
_critSect(*CriticalSectionWrapper::CreateCriticalSection()),
_vieCapture(ViECapture::GetInterface(videoEngine)),
_vieFile(ViEFile::GetInterface(videoEngine))
{
}

CaptureDevicePool::~CaptureDevicePool(void)
{
    assert(_deviceMap.Size()==0);
    _vieCapture->Release();
    _vieFile->Release();
    delete &_critSect;
}

WebRtc_Word32 CaptureDevicePool::GetCaptureDevice(int& captureId, const char* uniqeDeviceName)
{
    CriticalSectionScoped cs(_critSect);
    DeviceItem* device=NULL;
    
    for(MapItem* item=_deviceMap.First();
        item!=NULL;
        item=_deviceMap.Next(item))
    {
        
        if(strcmp(uniqeDeviceName,(static_cast<DeviceItem*>( item->GetItem()))->uniqeDeviceName)==0)
        {
            device=static_cast<DeviceItem*>( item->GetItem());
            device->refCount++;
            captureId=device->captureId;
            return 0;
        }
    }
    device = new DeviceItem;
    strncpy(device->uniqeDeviceName,uniqeDeviceName,255);


    
    WebRtc_Word32 result=_vieCapture->AllocateCaptureDevice(device->uniqeDeviceName,strlen(device->uniqeDeviceName),device->captureId);
    if(result==0)
    {
        result=_vieFile->SetCaptureDeviceImage(device->captureId,
                            "./main/test/WindowsTest/captureDeviceImage.jpg");
    }
    captureId=device->captureId;
    _deviceMap.Insert(captureId,device);
    device->refCount++;
    
    return result;


}
WebRtc_Word32 CaptureDevicePool::ReturnCaptureDevice(int captureId)
{
    CriticalSectionScoped cs(_critSect);

    MapItem* mapItem=_deviceMap.Find(captureId);
    if(!mapItem)
        return -1;

    DeviceItem* item=static_cast<DeviceItem*> (mapItem->GetItem());
    if(!item)
        return 0;
    item->refCount--;
    WebRtc_Word32 result=0;

    if(item->refCount==0)
    {
        result=_vieCapture->ReleaseCaptureDevice(captureId);
        
        _deviceMap.Erase(mapItem);
        delete item;

    }
    return result;
}
