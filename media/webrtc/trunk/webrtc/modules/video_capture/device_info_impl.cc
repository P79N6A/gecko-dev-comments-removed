









#include <stdlib.h>

#include "webrtc/modules/video_capture/device_info_impl.h"
#include "webrtc/modules/video_capture/video_capture_config.h"
#include "webrtc/system_wrappers/interface/trace.h"

#ifndef abs
#define abs(a) (a>=0?a:-a)
#endif

namespace webrtc
{
namespace videocapturemodule
{
DeviceInfoImpl::DeviceInfoImpl(const int32_t id)
    : _id(id), _apiLock(*RWLockWrapper::CreateRWLock()), _lastUsedDeviceName(NULL),
      _lastUsedDeviceNameLength(0)
{
}

DeviceInfoImpl::~DeviceInfoImpl(void)
{
    _apiLock.AcquireLockExclusive();
    free(_lastUsedDeviceName);
    _apiLock.ReleaseLockExclusive();

    delete &_apiLock;
}
int32_t DeviceInfoImpl::NumberOfCapabilities(
                                        const char* deviceUniqueIdUTF8)
{

    if (!deviceUniqueIdUTF8)
        return -1;

    _apiLock.AcquireLockShared();

    if (_lastUsedDeviceNameLength == strlen((char*) deviceUniqueIdUTF8))
    {
        
#if defined(WEBRTC_MAC) || defined(WEBRTC_LINUX) || defined(WEBRTC_BSD)
        if(strncasecmp((char*)_lastUsedDeviceName,
                       (char*) deviceUniqueIdUTF8,
                       _lastUsedDeviceNameLength)==0)
#else
        if (_strnicmp((char*) _lastUsedDeviceName,
                      (char*) deviceUniqueIdUTF8,
                      _lastUsedDeviceNameLength) == 0)
#endif
        {
            
            _apiLock.ReleaseLockShared();
            return static_cast<int32_t>(_captureCapabilities.size());
        }
    }
    
    _apiLock.ReleaseLockShared();
    WriteLockScoped cs2(_apiLock);

    int32_t ret = CreateCapabilityMap(deviceUniqueIdUTF8);
    return ret;
}

int32_t DeviceInfoImpl::GetCapability(const char* deviceUniqueIdUTF8,
                                      const uint32_t deviceCapabilityNumber,
                                      VideoCaptureCapability& capability)
{

    if (!deviceUniqueIdUTF8)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                   "deviceUniqueIdUTF8 parameter not set in call to GetCapability");
        return -1;
    }
    ReadLockScoped cs(_apiLock);

    if ((_lastUsedDeviceNameLength != strlen((char*) deviceUniqueIdUTF8))
#if defined(WEBRTC_MAC) || defined(WEBRTC_LINUX) || defined(WEBRTC_BSD)
        || (strncasecmp((char*)_lastUsedDeviceName,
                        (char*) deviceUniqueIdUTF8,
                        _lastUsedDeviceNameLength)!=0))
#else
        || (_strnicmp((char*) _lastUsedDeviceName,
                      (char*) deviceUniqueIdUTF8,
                      _lastUsedDeviceNameLength) != 0))
#endif

    {
        _apiLock.ReleaseLockShared();
        _apiLock.AcquireLockExclusive();
        if (-1 == CreateCapabilityMap(deviceUniqueIdUTF8))
        {
            _apiLock.ReleaseLockExclusive();
            _apiLock.AcquireLockShared();
            return -1;
        }
        _apiLock.ReleaseLockExclusive();
        _apiLock.AcquireLockShared();
    }

    
    if (deviceCapabilityNumber >= (unsigned int) _captureCapabilities.size())
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                   "deviceCapabilityNumber %d is invalid in call to GetCapability",
                   deviceCapabilityNumber);
        return -1;
    }

    capability = _captureCapabilities[deviceCapabilityNumber];
    return 0;
}

int32_t DeviceInfoImpl::GetBestMatchedCapability(
                                        const char*deviceUniqueIdUTF8,
                                        const VideoCaptureCapability& requested,
                                        VideoCaptureCapability& resulting)
{


    if (!deviceUniqueIdUTF8)
        return -1;

    ReadLockScoped cs(_apiLock);
    if ((_lastUsedDeviceNameLength != strlen((char*) deviceUniqueIdUTF8))
#if defined(WEBRTC_MAC) || defined(WEBRTC_LINUX) || defined(WEBRTC_BSD)
        || (strncasecmp((char*)_lastUsedDeviceName,
                        (char*) deviceUniqueIdUTF8,
                        _lastUsedDeviceNameLength)!=0))
#else
        || (_strnicmp((char*) _lastUsedDeviceName,
                      (char*) deviceUniqueIdUTF8,
                      _lastUsedDeviceNameLength) != 0))
#endif
    {
        _apiLock.ReleaseLockShared();
        _apiLock.AcquireLockExclusive();
        if (-1 == CreateCapabilityMap(deviceUniqueIdUTF8))
        {
            return -1;
        }
        _apiLock.ReleaseLockExclusive();
        _apiLock.AcquireLockShared();
    }

    int32_t bestformatIndex = -1;
    int32_t bestWidth = 0;
    int32_t bestHeight = 0;
    int32_t bestFrameRate = 0;
    RawVideoType bestRawType = kVideoUnknown;
    webrtc::VideoCodecType bestCodecType = webrtc::kVideoCodecUnknown;

    const int32_t numberOfCapabilies =
        static_cast<int32_t>(_captureCapabilities.size());

    for (int32_t tmp = 0; tmp < numberOfCapabilies; ++tmp) 
    {
        VideoCaptureCapability& capability = _captureCapabilities[tmp];

        const int32_t diffWidth = capability.width - requested.width;
        const int32_t diffHeight = capability.height - requested.height;
        const int32_t diffFrameRate = capability.maxFPS - requested.maxFPS;

        const int32_t currentbestDiffWith = bestWidth - requested.width;
        const int32_t currentbestDiffHeight = bestHeight - requested.height;
        const int32_t currentbestDiffFrameRate = bestFrameRate - requested.maxFPS;

        if ((diffHeight >= 0 && diffHeight <= abs(currentbestDiffHeight)) 
            || (currentbestDiffHeight < 0 && diffHeight >= currentbestDiffHeight))
        {

            if (diffHeight == currentbestDiffHeight) 
            {
                if ((diffWidth >= 0 && diffWidth <= abs(currentbestDiffWith)) 
                    || (currentbestDiffWith < 0 && diffWidth >= currentbestDiffWith))
                {
                    if (diffWidth == currentbestDiffWith && diffHeight
                        == currentbestDiffHeight) 
                    {
                        
                        if (((diffFrameRate >= 0 &&
                              diffFrameRate <= currentbestDiffFrameRate) 
                            ||
                            (currentbestDiffFrameRate < 0 &&
                             diffFrameRate >= currentbestDiffFrameRate)) 
                        )
                        {
                            if ((currentbestDiffFrameRate == diffFrameRate) 
                                || (currentbestDiffFrameRate >= 0))
                            {
                                if (bestRawType != requested.rawType
                                    && requested.rawType != kVideoUnknown
                                    && (capability.rawType == requested.rawType
                                        || capability.rawType == kVideoI420
                                        || capability.rawType == kVideoYUY2
                                        || capability.rawType == kVideoYV12))
                                {
                                    bestCodecType = capability.codecType;
                                    bestRawType = capability.rawType;
                                    bestformatIndex = tmp;
                                }
                                
                                if (capability.height == requested.height
                                    && capability.width == requested.width
                                    && capability.maxFPS >= requested.maxFPS)
                                {
                                    if (capability.codecType == requested.codecType
                                        && bestCodecType != requested.codecType)
                                    {
                                        bestCodecType = capability.codecType;
                                        bestformatIndex = tmp;
                                    }
                                }
                            }
                            else 
                            {
                                if (requested.codecType == capability.codecType)
                                {

                                    bestWidth = capability.width;
                                    bestHeight = capability.height;
                                    bestFrameRate = capability.maxFPS;
                                    bestCodecType = capability.codecType;
                                    bestRawType = capability.rawType;
                                    bestformatIndex = tmp;
                                }
                            }
                        }
                    }
                    else 
                    {
                        if (requested.codecType == capability.codecType)
                        {
                            bestWidth = capability.width;
                            bestHeight = capability.height;
                            bestFrameRate = capability.maxFPS;
                            bestCodecType = capability.codecType;
                            bestRawType = capability.rawType;
                            bestformatIndex = tmp;
                        }
                    }
                }
            }
            else 
            {
                if (requested.codecType == capability.codecType)
                {
                    bestWidth = capability.width;
                    bestHeight = capability.height;
                    bestFrameRate = capability.maxFPS;
                    bestCodecType = capability.codecType;
                    bestRawType = capability.rawType;
                    bestformatIndex = tmp;
                }
            }
        }
    }

    WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, _id,
               "Best camera format: Width %d, Height %d, Frame rate %d, Color format %d",
               bestWidth, bestHeight, bestFrameRate, bestRawType);

    
    if (bestformatIndex < 0)
        return -1;
    resulting = _captureCapabilities[bestformatIndex];
    return bestformatIndex;
}


int32_t DeviceInfoImpl::GetExpectedCaptureDelay(
                                          const DelayValues delayValues[],
                                          const uint32_t sizeOfDelayValues,
                                          const char* productId,
                                          const uint32_t width,
                                          const uint32_t height)
{
    int32_t bestDelay = kDefaultCaptureDelay;

    for (uint32_t device = 0; device < sizeOfDelayValues; ++device)
    {
        if (delayValues[device].productId && strncmp((char*) productId,
                                                     (char*) delayValues[device].productId,
                                                     kVideoCaptureProductIdLength) == 0)
        {
            

            int32_t bestWidth = 0;
            int32_t bestHeight = 0;

            
            for (uint32_t delayIndex = 0; delayIndex < NoOfDelayValues; ++delayIndex)
            {
                const DelayValue& currentValue = delayValues[device].delayValues[delayIndex];

                const int32_t diffWidth = currentValue.width - width;
                const int32_t diffHeight = currentValue.height - height;

                const int32_t currentbestDiffWith = bestWidth - width;
                const int32_t currentbestDiffHeight = bestHeight - height;

                if ((diffHeight >= 0 && diffHeight <= abs(currentbestDiffHeight)) 
                    || (currentbestDiffHeight < 0 && diffHeight >= currentbestDiffHeight))
                {

                    if (diffHeight == currentbestDiffHeight) 
                    {
                        if ((diffWidth >= 0 && diffWidth <= abs(currentbestDiffWith)) 
                            || (currentbestDiffWith < 0 && diffWidth >= currentbestDiffWith))
                        {
                            if (diffWidth == currentbestDiffWith && diffHeight
                                == currentbestDiffHeight) 
                            {
                            }
                            else 
                            {
                                bestWidth = currentValue.width;
                                bestHeight = currentValue.height;
                                bestDelay = currentValue.delay;
                            }
                        }
                    }
                    else 
                    {
                        bestWidth = currentValue.width;
                        bestHeight = currentValue.height;
                        bestDelay = currentValue.delay;
                    }
                }
            }
            break;
        }
    }
    if (bestDelay > kMaxCaptureDelay)
    {
        WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, _id,
                   "Expected capture delay too high. %dms, will use %d", bestDelay,
                   kMaxCaptureDelay);
        bestDelay = kMaxCaptureDelay;

    }

    return bestDelay;

}


int32_t DeviceInfoImpl::GetOrientation(const char* deviceUniqueIdUTF8,
                                       VideoCaptureRotation& orientation)
{
    orientation = kCameraRotate0;
    return -1;
}
}  
}  
