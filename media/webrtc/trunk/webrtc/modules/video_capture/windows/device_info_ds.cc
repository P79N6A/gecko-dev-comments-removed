









#include "webrtc/modules/video_capture/windows/device_info_ds.h"

#include "webrtc/modules/video_capture/video_capture_config.h"
#include "webrtc/modules/video_capture/video_capture_delay.h"
#include "webrtc/modules/video_capture/windows/help_functions_ds.h"
#include "webrtc/system_wrappers/interface/ref_count.h"
#include "webrtc/system_wrappers/interface/trace.h"

#include <Dvdmedia.h>

namespace webrtc
{
namespace videocapturemodule
{
const int32_t NoWindowsCaptureDelays = 1;
const DelayValues WindowsCaptureDelays[NoWindowsCaptureDelays] = {
  "Microsoft LifeCam Cinema",
  "usb#vid_045e&pid_075d",
  {
    {640,480,125},
    {640,360,117},
    {424,240,111},
    {352,288,111},
    {320,240,116},
    {176,144,101},
    {160,120,109},
    {1280,720,166},
    {960,544,126},
    {800,448,120},
    {800,600,127}
  },
};


void _FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0)
    {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL)
    {
        
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}


DeviceInfoDS* DeviceInfoDS::Create(const int32_t id)
{
    DeviceInfoDS* dsInfo = new DeviceInfoDS(id);
    if (!dsInfo || dsInfo->Init() != 0)
    {
        delete dsInfo;
        dsInfo = NULL;
    }
    return dsInfo;
}

DeviceInfoDS::DeviceInfoDS(const int32_t id)
    : DeviceInfoImpl(id), _dsDevEnum(NULL),
      _CoUninitializeIsRequired(true)
{
    
    
    
    
    
    
    
    
    
    

    






    
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED); 
    if (FAILED(hr))
    {
        
        _CoUninitializeIsRequired = FALSE;

        if (hr == RPC_E_CHANGED_MODE)
        {
            
            
            
            
            WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, _id,
                         "VideoCaptureWindowsDSInfo::VideoCaptureWindowsDSInfo "
                         "CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) => "
                         "RPC_E_CHANGED_MODE, error 0x%x",
                         hr);
        }
    }
}

DeviceInfoDS::~DeviceInfoDS()
{
    RELEASE_AND_CLEAR(_dsDevEnum);
    if (_CoUninitializeIsRequired)
    {
        CoUninitialize();
    }
}

int32_t DeviceInfoDS::Init()
{
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                                  IID_ICreateDevEnum, (void **) &_dsDevEnum);
    if (hr != NOERROR)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Failed to create CLSID_SystemDeviceEnum, error 0x%x", hr);
        return -1;
    }
    return 0;
}
uint32_t DeviceInfoDS::NumberOfDevices()
{
    ReadLockScoped cs(_apiLock);
    return GetDeviceInfo(0, 0, 0, 0, 0, 0, 0);
}

int32_t DeviceInfoDS::GetDeviceName(
                                       uint32_t deviceNumber,
                                       char* deviceNameUTF8,
                                       uint32_t deviceNameLength,
                                       char* deviceUniqueIdUTF8,
                                       uint32_t deviceUniqueIdUTF8Length,
                                       char* productUniqueIdUTF8,
                                       uint32_t productUniqueIdUTF8Length)
{
    ReadLockScoped cs(_apiLock);
    const int32_t result = GetDeviceInfo(deviceNumber, deviceNameUTF8,
                                         deviceNameLength,
                                         deviceUniqueIdUTF8,
                                         deviceUniqueIdUTF8Length,
                                         productUniqueIdUTF8,
                                         productUniqueIdUTF8Length);
    return result > (int32_t) deviceNumber ? 0 : -1;
}

int32_t DeviceInfoDS::GetDeviceInfo(
                                       uint32_t deviceNumber,
                                       char* deviceNameUTF8,
                                       uint32_t deviceNameLength,
                                       char* deviceUniqueIdUTF8,
                                       uint32_t deviceUniqueIdUTF8Length,
                                       char* productUniqueIdUTF8,
                                       uint32_t productUniqueIdUTF8Length)

{

    
    IEnumMoniker* _dsMonikerDevEnum = NULL;
    HRESULT hr =
        _dsDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
                                          &_dsMonikerDevEnum, 0);
    if (hr != NOERROR)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Failed to enumerate CLSID_SystemDeviceEnum, error 0x%x."
                     " No webcam exist?", hr);
        RELEASE_AND_CLEAR(_dsMonikerDevEnum);
        return 0;
    }

    _dsMonikerDevEnum->Reset();
    ULONG cFetched;
    IMoniker *pM;
    int index = 0;
    while (S_OK == _dsMonikerDevEnum->Next(1, &pM, &cFetched))
    {
        IPropertyBag *pBag;
        hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **) &pBag);
        if (S_OK == hr)
        {
            
            VARIANT varName;
            VariantInit(&varName);
            hr = pBag->Read(L"Description", &varName, 0);
            if (FAILED(hr))
            {
                hr = pBag->Read(L"FriendlyName", &varName, 0);
            }
            if (SUCCEEDED(hr))
            {
                
                if ((wcsstr(varName.bstrVal, (L"(VFW)")) == NULL) &&
                    (_wcsnicmp(varName.bstrVal, (L"Google Camera Adapter"),21)
                        != 0))
                {
                    
                    if (index == static_cast<int>(deviceNumber))
                    {
                        int convResult = 0;
                        if (deviceNameLength > 0)
                        {
                            convResult = WideCharToMultiByte(CP_UTF8, 0,
                                                             varName.bstrVal, -1,
                                                             (char*) deviceNameUTF8,
                                                             deviceNameLength, NULL,
                                                             NULL);
                            if (convResult == 0)
                            {
                                WEBRTC_TRACE(webrtc::kTraceError,
                                             webrtc::kTraceVideoCapture, _id,
                                             "Failed to convert device name to UTF8. %d",
                                             GetLastError());
                                RELEASE_AND_CLEAR(_dsMonikerDevEnum);
                                return -1;
                            }
                        }
                        if (deviceUniqueIdUTF8Length > 0)
                        {
                            hr = pBag->Read(L"DevicePath", &varName, 0);
                            if (FAILED(hr))
                            {
                                strncpy_s((char *) deviceUniqueIdUTF8,
                                          deviceUniqueIdUTF8Length,
                                          (char *) deviceNameUTF8, convResult);
                                WEBRTC_TRACE(webrtc::kTraceError,
                                             webrtc::kTraceVideoCapture, _id,
                                             "Failed to get deviceUniqueIdUTF8 using deviceNameUTF8");
                            }
                            else
                            {
                                convResult = WideCharToMultiByte(
                                                          CP_UTF8,
                                                          0,
                                                          varName.bstrVal,
                                                          -1,
                                                          (char*) deviceUniqueIdUTF8,
                                                          deviceUniqueIdUTF8Length,
                                                          NULL, NULL);
                                if (convResult == 0)
                                {
                                    WEBRTC_TRACE(webrtc::kTraceError,
                                                 webrtc::kTraceVideoCapture, _id,
                                                 "Failed to convert device name to UTF8. %d",
                                                 GetLastError());
                                    RELEASE_AND_CLEAR(_dsMonikerDevEnum);
                                    return -1;
                                }
                                if (productUniqueIdUTF8
                                    && productUniqueIdUTF8Length > 0)
                                {
                                    GetProductId(deviceUniqueIdUTF8,
                                                 productUniqueIdUTF8,
                                                 productUniqueIdUTF8Length);
                                }
                            }
                        }

                    }
                    ++index; 
                }
            }
            VariantClear(&varName);
            pBag->Release();
            pM->Release();
        }

    }
    if (deviceNameLength)
    {
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, _id, "%s %s",
                     __FUNCTION__, deviceNameUTF8);
    }
    RELEASE_AND_CLEAR(_dsMonikerDevEnum);
    return index;
}

IBaseFilter * DeviceInfoDS::GetDeviceFilter(
                                     const char* deviceUniqueIdUTF8,
                                     char* productUniqueIdUTF8,
                                     uint32_t productUniqueIdUTF8Length)
{

    const int32_t deviceUniqueIdUTF8Length =
        (int32_t) strlen((char*) deviceUniqueIdUTF8); 
    if (deviceUniqueIdUTF8Length > kVideoCaptureUniqueNameLength)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Device name too long");
        return NULL;
    }

    IEnumMoniker* _dsMonikerDevEnum = NULL;
    
    HRESULT hr = _dsDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
                                                   &_dsMonikerDevEnum, 0);
    if (hr != NOERROR)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Failed to enumerate CLSID_SystemDeviceEnum, error 0x%x."
                     " No webcam exist?", hr);
        RELEASE_AND_CLEAR(_dsMonikerDevEnum);
        return 0;
    }
    _dsMonikerDevEnum->Reset();
    ULONG cFetched;
    IMoniker *pM;

    IBaseFilter *captureFilter = NULL;
    bool deviceFound = false;
    while (S_OK == _dsMonikerDevEnum->Next(1, &pM, &cFetched) && !deviceFound)
    {
        IPropertyBag *pBag;
        hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **) &pBag);
        if (S_OK == hr)
        {
            
            VARIANT varName;
            VariantInit(&varName);
            if (deviceUniqueIdUTF8Length > 0)
            {
                hr = pBag->Read(L"DevicePath", &varName, 0);
                if (FAILED(hr))
                {
                    hr = pBag->Read(L"Description", &varName, 0);
                    if (FAILED(hr))
                    {
                        hr = pBag->Read(L"FriendlyName", &varName, 0);
                    }
                }
                if (SUCCEEDED(hr))
                {
                    char tempDevicePathUTF8[256];
                    tempDevicePathUTF8[0] = 0;
                    WideCharToMultiByte(CP_UTF8, 0, varName.bstrVal, -1,
                                        tempDevicePathUTF8,
                                        sizeof(tempDevicePathUTF8), NULL,
                                        NULL);
                    if (strncmp(tempDevicePathUTF8,
                                (const char*) deviceUniqueIdUTF8,
                                deviceUniqueIdUTF8Length) == 0)
                    {
                        
                        deviceFound = true;
                        hr = pM->BindToObject(0, 0, IID_IBaseFilter,
                                              (void**) &captureFilter);
                        if FAILED(hr)
                        {
                            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture,
                                         _id, "Failed to bind to the selected capture device %d",hr);
                        }

                        if (productUniqueIdUTF8
                            && productUniqueIdUTF8Length > 0) 
                        {

                            GetProductId(deviceUniqueIdUTF8,
                                         productUniqueIdUTF8,
                                         productUniqueIdUTF8Length);
                        }

                    }
                }
            }
            VariantClear(&varName);
            pBag->Release();
            pM->Release();
        }
    }
    RELEASE_AND_CLEAR(_dsMonikerDevEnum);
    return captureFilter;
}

int32_t DeviceInfoDS::GetWindowsCapability(
    const int32_t capabilityIndex,
    VideoCaptureCapabilityWindows& windowsCapability) {
  ReadLockScoped cs(_apiLock);

  if (capabilityIndex < 0 || static_cast<size_t>(capabilityIndex) >=
                                 _captureCapabilitiesWindows.size()) {
    return -1;
  }

  windowsCapability = _captureCapabilitiesWindows[capabilityIndex];
  return 0;
}

int32_t DeviceInfoDS::CreateCapabilityMap(
                                         const char* deviceUniqueIdUTF8)

{
    
    _captureCapabilities.clear();

    const int32_t deviceUniqueIdUTF8Length =
        (int32_t) strlen((char*) deviceUniqueIdUTF8);
    if (deviceUniqueIdUTF8Length > kVideoCaptureUniqueNameLength)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Device name too long");
        return -1;
    }
    WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, _id,
                 "CreateCapabilityMap called for device %s", deviceUniqueIdUTF8);


    char productId[kVideoCaptureProductIdLength];
    IBaseFilter* captureDevice = DeviceInfoDS::GetDeviceFilter(
                                               deviceUniqueIdUTF8,
                                               productId,
                                               kVideoCaptureProductIdLength);
    if (!captureDevice)
        return -1;
    IPin* outputCapturePin = GetOutputPin(captureDevice, GUID_NULL);
    if (!outputCapturePin)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Failed to get capture device output pin");
        RELEASE_AND_CLEAR(captureDevice);
        return -1;
    }
    IAMExtDevice* extDevice = NULL;
    HRESULT hr = captureDevice->QueryInterface(IID_IAMExtDevice,
                                               (void **) &extDevice);
    if (SUCCEEDED(hr) && extDevice)
    {
        WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, _id,
                     "This is an external device");
        extDevice->Release();
    }

    IAMStreamConfig* streamConfig = NULL;
    hr = outputCapturePin->QueryInterface(IID_IAMStreamConfig,
                                          (void**) &streamConfig);
    if (FAILED(hr))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Failed to get IID_IAMStreamConfig interface from capture device");
        return -1;
    }

    
    IAMVideoControl* videoControlConfig = NULL;
    HRESULT hrVC = captureDevice->QueryInterface(IID_IAMVideoControl,
                                      (void**) &videoControlConfig);
    if (FAILED(hrVC))
    {
        WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, _id,
                     "IID_IAMVideoControl Interface NOT SUPPORTED");
    }

    AM_MEDIA_TYPE *pmt = NULL;
    VIDEO_STREAM_CONFIG_CAPS caps;
    int count, size;

    hr = streamConfig->GetNumberOfCapabilities(&count, &size);
    if (FAILED(hr))
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                     "Failed to GetNumberOfCapabilities");
        RELEASE_AND_CLEAR(videoControlConfig);
        RELEASE_AND_CLEAR(streamConfig);
        RELEASE_AND_CLEAR(outputCapturePin);
        RELEASE_AND_CLEAR(captureDevice);
        return -1;
    }

    
    
    
    bool supportFORMAT_VideoInfo2 = false;
    bool supportFORMAT_VideoInfo = false;
    bool foundInterlacedFormat = false;
    GUID preferedVideoFormat = FORMAT_VideoInfo;
    for (int32_t tmp = 0; tmp < count; ++tmp)
    {
        hr = streamConfig->GetStreamCaps(tmp, &pmt,
                                         reinterpret_cast<BYTE*> (&caps));
        
        if (!FAILED(hr) && pmt)
        {
            if (pmt->majortype == MEDIATYPE_Video
                && pmt->formattype == FORMAT_VideoInfo2)
            {
                WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, _id,
                             " Device support FORMAT_VideoInfo2");
                supportFORMAT_VideoInfo2 = true;
                VIDEOINFOHEADER2* h =
                    reinterpret_cast<VIDEOINFOHEADER2*> (pmt->pbFormat);
                assert(h);
                foundInterlacedFormat |= h->dwInterlaceFlags
                                        & (AMINTERLACE_IsInterlaced
                                           | AMINTERLACE_DisplayModeBobOnly);
            }
            if (pmt->majortype == MEDIATYPE_Video
                && pmt->formattype == FORMAT_VideoInfo)
            {
                WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCapture, _id,
                             " Device support FORMAT_VideoInfo2");
                supportFORMAT_VideoInfo = true;
            }
        }
    }
    if (supportFORMAT_VideoInfo2)
    {
        if (supportFORMAT_VideoInfo && !foundInterlacedFormat)
        {
            preferedVideoFormat = FORMAT_VideoInfo;
        }
        else
        {
            preferedVideoFormat = FORMAT_VideoInfo2;
        }
    }

    for (int32_t tmp = 0; tmp < count; ++tmp)
    {
        hr = streamConfig->GetStreamCaps(tmp, &pmt,
                                         reinterpret_cast<BYTE*> (&caps));
        if (FAILED(hr))
        {
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, _id,
                         "Failed to GetStreamCaps");
            RELEASE_AND_CLEAR(videoControlConfig);
            RELEASE_AND_CLEAR(streamConfig);
            RELEASE_AND_CLEAR(outputCapturePin);
            RELEASE_AND_CLEAR(captureDevice);
            return -1;
        }

        if (pmt->majortype == MEDIATYPE_Video
            && pmt->formattype == preferedVideoFormat)
        {

            VideoCaptureCapabilityWindows capability;
            int64_t avgTimePerFrame = 0;

            if (pmt->formattype == FORMAT_VideoInfo)
            {
                VIDEOINFOHEADER* h =
                    reinterpret_cast<VIDEOINFOHEADER*> (pmt->pbFormat);
                assert(h);
                capability.directShowCapabilityIndex = tmp;
                capability.width = h->bmiHeader.biWidth;
                capability.height = h->bmiHeader.biHeight;
                avgTimePerFrame = h->AvgTimePerFrame;
            }
            if (pmt->formattype == FORMAT_VideoInfo2)
            {
                VIDEOINFOHEADER2* h =
                    reinterpret_cast<VIDEOINFOHEADER2*> (pmt->pbFormat);
                assert(h);
                capability.directShowCapabilityIndex = tmp;
                capability.width = h->bmiHeader.biWidth;
                capability.height = h->bmiHeader.biHeight;
                capability.interlaced = h->dwInterlaceFlags
                                        & (AMINTERLACE_IsInterlaced
                                           | AMINTERLACE_DisplayModeBobOnly);
                avgTimePerFrame = h->AvgTimePerFrame;
            }

            if (hrVC == S_OK)
            {
                LONGLONG *frameDurationList = NULL;
                LONGLONG maxFPS;
                long listSize;
                SIZE size;
                size.cx = capability.width;
                size.cy = capability.height;

                
                
                
                
                
                hrVC = videoControlConfig->GetFrameRateList(outputCapturePin,
                                                            tmp, size,
                                                            &listSize,
                                                            &frameDurationList);

                
                
                
                
                if (hrVC == S_OK && listSize > 0 && frameDurationList &&
                    0 != (maxFPS = GetMaxOfFrameArray(frameDurationList,
                                                      listSize)))
                {
                    capability.maxFPS = static_cast<int> (10000000
                                                           / maxFPS);
                    capability.supportFrameRateControl = true;
                }
                else 
                {
                    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture,
                                 _id,
                                 "GetMaxAvailableFrameRate NOT SUPPORTED");
                    if (avgTimePerFrame > 0)
                        capability.maxFPS = static_cast<int> (10000000
                                                               / avgTimePerFrame);
                    else
                        capability.maxFPS = 0;
                }

                if (frameDurationList) {
                  CoTaskMemFree((PVOID)frameDurationList); 
                }
            }
            else 
            {
                if (avgTimePerFrame > 0)
                    capability.maxFPS = static_cast<int> (10000000
                                                           / avgTimePerFrame);
                else
                    capability.maxFPS = 0;
            }

            
            if (pmt->subtype == MEDIASUBTYPE_I420)
            {
                capability.rawType = kVideoI420;
            }
            else if (pmt->subtype == MEDIASUBTYPE_IYUV)
            {
                capability.rawType = kVideoIYUV;
            }
            else if (pmt->subtype == MEDIASUBTYPE_RGB24)
            {
                capability.rawType = kVideoRGB24;
            }
            else if (pmt->subtype == MEDIASUBTYPE_YUY2)
            {
                capability.rawType = kVideoYUY2;
            }
            else if (pmt->subtype == MEDIASUBTYPE_RGB565)
            {
                capability.rawType = kVideoRGB565;
            }
            else if (pmt->subtype == MEDIASUBTYPE_MJPG)
            {
                capability.rawType = kVideoMJPEG;
            }
            else if (pmt->subtype == MEDIASUBTYPE_dvsl
                    || pmt->subtype == MEDIASUBTYPE_dvsd
                    || pmt->subtype == MEDIASUBTYPE_dvhd) 
            {
                capability.rawType = kVideoYUY2;
            }
            else if (pmt->subtype == MEDIASUBTYPE_UYVY) 
            {
                capability.rawType = kVideoUYVY;
            }
            else if (pmt->subtype == MEDIASUBTYPE_HDYC) 
            {
                WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideoCapture, _id,
                             "Device support HDYC.");
                capability.rawType = kVideoUYVY;
            }
            else
            {
                WCHAR strGuid[39];
                StringFromGUID2(pmt->subtype, strGuid, 39);
                WEBRTC_TRACE( webrtc::kTraceWarning,
                             webrtc::kTraceVideoCapture, _id,
                             "Device supports unknown media type %ls",
                             strGuid);
                
                assert(capability.rawType == kVideoUnknown);
            }

            if (capability.rawType != kVideoUnknown) {
              
              capability.expectedCaptureDelay
                = GetExpectedCaptureDelay(WindowsCaptureDelays,
                                          NoWindowsCaptureDelays,
                                          productId,
                                          capability.width,
                                          capability.height);
              _captureCapabilities.push_back(capability);
              _captureCapabilitiesWindows.push_back(capability);
              WEBRTC_TRACE( webrtc::kTraceInfo, webrtc::kTraceVideoCapture, _id,
                            "Camera capability, width:%d height:%d type:%d fps:%d",
                            capability.width, capability.height,
                            capability.rawType, capability.maxFPS);
            }
        }
        _FreeMediaType(*pmt);
        pmt = NULL;
    }
    RELEASE_AND_CLEAR(streamConfig);
    RELEASE_AND_CLEAR(videoControlConfig);
    RELEASE_AND_CLEAR(outputCapturePin);
    RELEASE_AND_CLEAR(captureDevice); 

    
    _lastUsedDeviceNameLength = deviceUniqueIdUTF8Length;
    _lastUsedDeviceName = (char*) realloc(_lastUsedDeviceName,
                                                   _lastUsedDeviceNameLength
                                                       + 1);
    memcpy(_lastUsedDeviceName, deviceUniqueIdUTF8, _lastUsedDeviceNameLength+ 1);
    WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCapture, _id,
                 "CreateCapabilityMap %d", _captureCapabilities.size());

    return static_cast<int32_t>(_captureCapabilities.size());
}







void DeviceInfoDS::GetProductId(const char* devicePath,
                                      char* productUniqueIdUTF8,
                                      uint32_t productUniqueIdUTF8Length)
{
    *productUniqueIdUTF8 = '\0';
    char* startPos = strstr((char*) devicePath, "\\\\?\\");
    if (!startPos)
    {
        strncpy_s((char*) productUniqueIdUTF8, productUniqueIdUTF8Length, "", 1);
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                     "Failed to get the product Id");
        return;
    }
    startPos += 4;

    char* pos = strchr(startPos, '&');
    if (!pos || pos >= (char*) devicePath + strlen((char*) devicePath))
    {
        strncpy_s((char*) productUniqueIdUTF8, productUniqueIdUTF8Length, "", 1);
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                     "Failed to get the product Id");
        return;
    }
    
    pos = strchr(pos + 1, '&');
    uint32_t bytesToCopy = (uint32_t)(pos - startPos);
    if (pos && (bytesToCopy <= productUniqueIdUTF8Length) && bytesToCopy
        <= kVideoCaptureProductIdLength)
    {
        strncpy_s((char*) productUniqueIdUTF8, productUniqueIdUTF8Length,
                  (char*) startPos, bytesToCopy);
    }
    else
    {
        strncpy_s((char*) productUniqueIdUTF8, productUniqueIdUTF8Length, "", 1);
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, -1,
                     "Failed to get the product Id");
    }
}

int32_t DeviceInfoDS::DisplayCaptureSettingsDialogBox(
                                         const char* deviceUniqueIdUTF8,
                                         const char* dialogTitleUTF8,
                                         void* parentWindow,
                                         uint32_t positionX,
                                         uint32_t positionY)
{
    ReadLockScoped cs(_apiLock);
    HWND window = (HWND) parentWindow;

    IBaseFilter* filter = GetDeviceFilter(deviceUniqueIdUTF8, NULL, 0);
    if (!filter)
        return -1;

    ISpecifyPropertyPages* pPages = NULL;
    CAUUID uuid;
    HRESULT hr = S_OK;

    hr = filter->QueryInterface(IID_ISpecifyPropertyPages, (LPVOID*) &pPages);
    if (!SUCCEEDED(hr))
    {
        filter->Release();
        return -1;
    }
    hr = pPages->GetPages(&uuid);
    if (!SUCCEEDED(hr))
    {
        filter->Release();
        return -1;
    }

    WCHAR tempDialogTitleWide[256];
    tempDialogTitleWide[0] = 0;
    int size = 255;

    
    MultiByteToWideChar(CP_UTF8, 0, (char*) dialogTitleUTF8, -1,
                        tempDialogTitleWide, size);

    

    hr = OleCreatePropertyFrame(window, 
                                positionX, 
                                positionY, 
                                tempDialogTitleWide,
                                1, 
                                (LPUNKNOWN*) &filter, 
                                uuid.cElems, 
                                uuid.pElems, 
                                LOCALE_USER_DEFAULT, 
                                0, NULL); 
    
    if (uuid.pElems)
    {
        CoTaskMemFree(uuid.pElems);
    }
    filter->Release();
    return 0;
}
}  
}  
