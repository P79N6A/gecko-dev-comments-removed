









#include "engine_configurations.h"
#include "video_render_windows_impl.h"

#include "critical_section_wrapper.h"
#include "trace.h"
#ifdef DIRECTDRAW_RENDERING
#include "video_render_directdraw.h"
#endif
#ifdef DIRECT3D9_RENDERING
#include "video_render_direct3d9.h"
#endif

#include <tchar.h>

namespace webrtc {

VideoRenderWindowsImpl::VideoRenderWindowsImpl(
                                               const WebRtc_Word32 id,
                                               const VideoRenderType videoRenderType,
                                               void* window,
                                               const bool fullscreen) :
            _id(id),
            _renderWindowsCritsect(
                                   *CriticalSectionWrapper::CreateCriticalSection()),
            _prtWindow(window), _fullscreen(fullscreen), _ptrRendererWin(NULL)
{
}

VideoRenderWindowsImpl::~VideoRenderWindowsImpl()
{
    delete &_renderWindowsCritsect;
    if (_ptrRendererWin)
    {
        delete _ptrRendererWin;
        _ptrRendererWin = NULL;
    }
}

WebRtc_Word32 VideoRenderWindowsImpl::Init()
{
    
    CheckHWAcceleration();

    _renderMethod = kVideoRenderWinD3D9;

    
    switch (_renderMethod)
    {
        case kVideoRenderWinDd:
        {
#ifdef DIRECTDRAW_RENDERING
            VideoRenderDirectDraw* ptrRenderer;
            ptrRenderer = new VideoRenderDirectDraw(NULL, (HWND) _prtWindow, _fullscreen);
            if (ptrRenderer == NULL)
            {
                break;
            }
            _ptrRendererWin = reinterpret_cast<IVideoRenderWin*>(ptrRenderer);
#else
            return NULL;
#endif  
        }
            break;
        case kVideoRenderWinD3D9:
        {
#ifdef DIRECT3D9_RENDERING
            VideoRenderDirect3D9* ptrRenderer;
            ptrRenderer = new VideoRenderDirect3D9(NULL, (HWND) _prtWindow, _fullscreen);
            if (ptrRenderer == NULL)
            {
                break;
            }
            _ptrRendererWin = reinterpret_cast<IVideoRenderWin*>(ptrRenderer);
#else
            return NULL;
#endif  
        }
            break;
        default:
            break;
    }

    
    if (_ptrRendererWin)
        return _ptrRendererWin->Init();
    else
        return -1;
}

WebRtc_Word32 VideoRenderWindowsImpl::ChangeUniqueId(const WebRtc_Word32 id)
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    _id = id;
    return 0;
}

WebRtc_Word32 VideoRenderWindowsImpl::ChangeWindow(void* window)
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    if (!_ptrRendererWin)
    {
        return -1;
    }
    else
    {
        return _ptrRendererWin->ChangeWindow(window);
    }
}

VideoRenderCallback*
VideoRenderWindowsImpl::AddIncomingRenderStream(const WebRtc_UWord32 streamId,
                                                const WebRtc_UWord32 zOrder,
                                                const float left,
                                                const float top,
                                                const float right,
                                                const float bottom)
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    VideoRenderCallback* renderCallback = NULL;

    if (!_ptrRendererWin)
    {
    }
    else
    {
        renderCallback = _ptrRendererWin->CreateChannel(streamId, zOrder, left,
                                                        top, right, bottom);
    }

    return renderCallback;
}

WebRtc_Word32 VideoRenderWindowsImpl::DeleteIncomingRenderStream(
                                                                 const WebRtc_UWord32 streamId)
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    WebRtc_Word32 error = -1;
    if (!_ptrRendererWin)
    {
    }
    else
    {
        error = _ptrRendererWin->DeleteChannel(streamId);
    }
    return error;
}

WebRtc_Word32 VideoRenderWindowsImpl::GetIncomingRenderStreamProperties(
                                                                        const WebRtc_UWord32 streamId,
                                                                        WebRtc_UWord32& zOrder,
                                                                        float& left,
                                                                        float& top,
                                                                        float& right,
                                                                        float& bottom) const
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    zOrder = 0;
    left = 0;
    top = 0;
    right = 0;
    bottom = 0;

    WebRtc_Word32 error = -1;
    if (!_ptrRendererWin)
    {
    }
    else
    {
        error = _ptrRendererWin->GetStreamSettings(streamId, 0, zOrder, left,
                                                   top, right, bottom);
    }
    return error;
}

WebRtc_Word32 VideoRenderWindowsImpl::StartRender()
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    WebRtc_Word32 error = -1;
    if (!_ptrRendererWin)
    {
    }
    else
    {
        error = _ptrRendererWin->StartRender();
    }
    return error;
}

WebRtc_Word32 VideoRenderWindowsImpl::StopRender()
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    WebRtc_Word32 error = -1;
    if (!_ptrRendererWin)
    {
    }
    else
    {
        error = _ptrRendererWin->StopRender();
    }
    return error;
}

VideoRenderType VideoRenderWindowsImpl::RenderType()
{
    return kRenderWindows;
}

RawVideoType VideoRenderWindowsImpl::PerferedVideoType()
{
    return kVideoI420;
}

bool VideoRenderWindowsImpl::FullScreen()
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    bool fullscreen = false;
    if (!_ptrRendererWin)
    {
    }
    else
    {
        fullscreen = _ptrRendererWin->IsFullScreen();
    }
    return fullscreen;
}

WebRtc_Word32 VideoRenderWindowsImpl::GetGraphicsMemory(
                                                        WebRtc_UWord64& totalGraphicsMemory,
                                                        WebRtc_UWord64& availableGraphicsMemory) const
{
    if (_ptrRendererWin)
    {
        return _ptrRendererWin->GetGraphicsMemory(totalGraphicsMemory,
                                                  availableGraphicsMemory);
    }

    totalGraphicsMemory = 0;
    availableGraphicsMemory = 0;
    return -1;
}

WebRtc_Word32 VideoRenderWindowsImpl::GetScreenResolution(
                                                          WebRtc_UWord32& screenWidth,
                                                          WebRtc_UWord32& screenHeight) const
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    screenWidth = 0;
    screenHeight = 0;
    return 0;
}

WebRtc_UWord32 VideoRenderWindowsImpl::RenderFrameRate(
                                                       const WebRtc_UWord32 streamId)
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    return 0;
}

WebRtc_Word32 VideoRenderWindowsImpl::SetStreamCropping(
                                                        const WebRtc_UWord32 streamId,
                                                        const float left,
                                                        const float top,
                                                        const float right,
                                                        const float bottom)
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    WebRtc_Word32 error = -1;
    if (!_ptrRendererWin)
    {
    }
    else
    {
        error = _ptrRendererWin->SetCropping(streamId, 0, left, top, right,
                                             bottom);
    }
    return error;
}

WebRtc_Word32 VideoRenderWindowsImpl::ConfigureRenderer(
                                                        const WebRtc_UWord32 streamId,
                                                        const unsigned int zOrder,
                                                        const float left,
                                                        const float top,
                                                        const float right,
                                                        const float bottom)
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    WebRtc_Word32 error = -1;
    if (!_ptrRendererWin)
    {
    }
    else
    {
        error = _ptrRendererWin->ConfigureRenderer(streamId, 0, zOrder, left,
                                                   top, right, bottom);
    }

    return error;
}

WebRtc_Word32 VideoRenderWindowsImpl::SetTransparentBackground(
                                                               const bool enable)
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    WebRtc_Word32 error = -1;
    if (!_ptrRendererWin)
    {
    }
    else
    {
        error = _ptrRendererWin->SetTransparentBackground(enable);
    }
    return error;
}

WebRtc_Word32 VideoRenderWindowsImpl::SetText(
                                              const WebRtc_UWord8 textId,
                                              const WebRtc_UWord8* text,
                                              const WebRtc_Word32 textLength,
                                              const WebRtc_UWord32 textColorRef,
                                              const WebRtc_UWord32 backgroundColorRef,
                                              const float left,
                                              const float top,
                                              const float right,
                                              const float bottom)
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    WebRtc_Word32 error = -1;
    if (!_ptrRendererWin)
    {
    }
    else
    {
        error = _ptrRendererWin->SetText(textId, text, textLength,
                                         textColorRef, backgroundColorRef,
                                         left, top, right, bottom);
    }
    return error;
}

WebRtc_Word32 VideoRenderWindowsImpl::SetBitmap(const void* bitMap,
                                                const WebRtc_UWord8 pictureId,
                                                const void* colorKey,
                                                const float left,
                                                const float top,
                                                const float right,
                                                const float bottom)
{
    CriticalSectionScoped cs(&_renderWindowsCritsect);
    WebRtc_Word32 error = -1;
    if (!_ptrRendererWin)
    {
    }
    else
    {
        error = _ptrRendererWin->SetBitmap(bitMap, pictureId, colorKey, left,
                                           top, right, bottom);
    }
    return error;
}

void VideoRenderWindowsImpl::LogOSAndHardwareDetails()
{
    HRESULT hr;
    IDxDiagProvider* m_pDxDiagProvider = NULL;
    IDxDiagContainer* m_pDxDiagRoot = NULL;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    bool coUninitializeIsRequired = true;
    if (FAILED(hr))
    {
        
        coUninitializeIsRequired = false;
        if (hr == RPC_E_CHANGED_MODE)
        {
            
            
            
            
            WEBRTC_TRACE(
                         kTraceWarning,
                         kTraceVideoRenderer,
                         _id,
                         "VideoRenderWindowsImpl::LogOSAndHardwareDetails() CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) => RPC_E_CHANGED_MODE, error 0x%x",
                         hr);
        }
    }

    hr = CoCreateInstance(CLSID_DxDiagProvider, NULL, CLSCTX_INPROC_SERVER,
                          IID_IDxDiagProvider, (LPVOID*) &m_pDxDiagProvider);

    if (FAILED(hr) || m_pDxDiagProvider == NULL)
    {
        if (coUninitializeIsRequired)
            CoUninitialize();
        return;
    }

    
    
    
    
    DXDIAG_INIT_PARAMS dxDiagInitParam;
    ZeroMemory(&dxDiagInitParam, sizeof(DXDIAG_INIT_PARAMS));

    dxDiagInitParam.dwSize = sizeof(DXDIAG_INIT_PARAMS);
    dxDiagInitParam.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
    dxDiagInitParam.bAllowWHQLChecks = TRUE;
    dxDiagInitParam.pReserved = NULL;

    hr = m_pDxDiagProvider->Initialize(&dxDiagInitParam);
    if (FAILED(hr))
    {
        m_pDxDiagProvider->Release();
        if (coUninitializeIsRequired)
            CoUninitialize();
        return;
    }

    hr = m_pDxDiagProvider->GetRootContainer(&m_pDxDiagRoot);
    if (FAILED(hr) || m_pDxDiagRoot == NULL)
    {
        m_pDxDiagProvider->Release();
        if (coUninitializeIsRequired)
            CoUninitialize();
        return;
    }

    IDxDiagContainer* pObject = NULL;

    hr = m_pDxDiagRoot->GetChildContainer(L"DxDiag_SystemInfo", &pObject);
    if (FAILED(hr) || pObject == NULL)
    {
        m_pDxDiagRoot->Release();
        m_pDxDiagProvider->Release();
        if (coUninitializeIsRequired)
            CoUninitialize();
        return;
    }

    TCHAR m_szDirectXVersionLongEnglish[100];
    TCHAR m_szOSLocalized[100];
    TCHAR m_szProcessorEnglish[200];
    TCHAR m_szSystemManufacturerEnglish[200];

    ZeroMemory(m_szDirectXVersionLongEnglish, sizeof(TCHAR) * 100);
    ZeroMemory(m_szOSLocalized, sizeof(TCHAR) * 100);
    ZeroMemory(m_szProcessorEnglish, sizeof(TCHAR) * 200);
    ZeroMemory(m_szSystemManufacturerEnglish, sizeof(TCHAR) * 200);

    GetStringValue( pObject, L"szDirectXVersionLongEnglish",
                   EXPAND(m_szDirectXVersionLongEnglish) );
    GetStringValue(pObject, L"szOSLocalized", EXPAND(m_szOSLocalized) );
    GetStringValue(pObject, L"szProcessorEnglish", EXPAND(m_szProcessorEnglish) );
    GetStringValue( pObject, L"szSystemManufacturerEnglish",
                   EXPAND(m_szSystemManufacturerEnglish) );

    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "System Manufacturer             --- %s",
                 m_szSystemManufacturerEnglish);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "Processor                       --- %s", m_szProcessorEnglish);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "Operating System                --- %s", m_szOSLocalized);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "DirectX Version                 --- %s",
                 m_szDirectXVersionLongEnglish);

    if (pObject)
        pObject->Release();

    struct DisplayInfo
    {
        TCHAR m_szDescription[200];
        TCHAR m_szManufacturer[200];
        TCHAR m_szChipType[100];
        TCHAR m_szDisplayMemoryEnglish[100];
        TCHAR m_szDisplayModeEnglish[100];
        TCHAR m_szDriverName[100];
        TCHAR m_szDriverVersion[100];
        TCHAR m_szDDStatusEnglish[100];
        TCHAR m_szD3DStatusEnglish[100];
        BOOL m_bDDAccelerationEnabled;
        BOOL m_bNoHardware;
        BOOL m_b3DAccelerationExists;
        BOOL m_b3DAccelerationEnabled;
    };

    WCHAR wszContainer[256];
    IDxDiagContainer* pContainer = NULL;

    DWORD nInstanceCount = 0;
    DWORD nItem = 0;

    
    
    if (FAILED(hr = m_pDxDiagRoot->GetChildContainer(L"DxDiag_DisplayDevices",
                                                     &pContainer)))
    {
        m_pDxDiagRoot->Release();
        m_pDxDiagProvider->Release();
        if (coUninitializeIsRequired)
            CoUninitialize();
        return;
    }

    if (FAILED(hr = pContainer->GetNumberOfChildContainers(&nInstanceCount)))
    {
        pContainer->Release();
        m_pDxDiagRoot->Release();
        m_pDxDiagProvider->Release();
        if (coUninitializeIsRequired)
            CoUninitialize();
        return;
    }

    DisplayInfo *pDisplayInfo = new DisplayInfo;
    if (pDisplayInfo == NULL)
        return;
    ZeroMemory(pDisplayInfo, sizeof(DisplayInfo));

    hr = pContainer->EnumChildContainerNames(nItem, wszContainer, 256);
    if (FAILED(hr))
    {
        delete pDisplayInfo;
        pContainer->Release();
        m_pDxDiagRoot->Release();
        m_pDxDiagProvider->Release();
        if (coUninitializeIsRequired)
            CoUninitialize();
        return;
    }

    hr = pContainer->GetChildContainer(wszContainer, &pObject);
    if (FAILED(hr) || pObject == NULL)
    {
        delete pDisplayInfo;
        pContainer->Release();
        m_pDxDiagRoot->Release();
        m_pDxDiagProvider->Release();
        if (coUninitializeIsRequired)
            CoUninitialize();
        return;
    }

    GetStringValue( pObject, L"szDescription",
                   EXPAND(pDisplayInfo->m_szDescription) );
    GetStringValue( pObject, L"szManufacturer",
                   EXPAND(pDisplayInfo->m_szManufacturer) );
    GetStringValue(pObject, L"szChipType", EXPAND(pDisplayInfo->m_szChipType) );
    GetStringValue( pObject, L"szDisplayMemoryEnglish",
                   EXPAND(pDisplayInfo->m_szDisplayMemoryEnglish) );
    GetStringValue( pObject, L"szDisplayModeEnglish",
                   EXPAND(pDisplayInfo->m_szDisplayModeEnglish) );
    GetStringValue( pObject, L"szDriverName",
                   EXPAND(pDisplayInfo->m_szDriverName) );
    GetStringValue( pObject, L"szDriverVersion",
                   EXPAND(pDisplayInfo->m_szDriverVersion) );
    GetBoolValue(pObject, L"bDDAccelerationEnabled",
                 &pDisplayInfo->m_bDDAccelerationEnabled);
    GetBoolValue(pObject, L"bNoHardware", &pDisplayInfo->m_bNoHardware);
    GetBoolValue(pObject, L"bDDAccelerationEnabled",
                 &pDisplayInfo->m_bDDAccelerationEnabled);
    GetBoolValue(pObject, L"b3DAccelerationExists",
                 &pDisplayInfo->m_b3DAccelerationExists);
    GetBoolValue(pObject, L"b3DAccelerationEnabled",
                 &pDisplayInfo->m_b3DAccelerationEnabled);
    GetStringValue( pObject, L"szDDStatusEnglish",
                   EXPAND(pDisplayInfo->m_szDDStatusEnglish));
    GetStringValue( pObject, L"szD3DStatusEnglish",
                   EXPAND(pDisplayInfo->m_szD3DStatusEnglish));

    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "Device Name                     --- %s",
                 pDisplayInfo->m_szDescription);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "Device Manufacturer             --- %s",
                 pDisplayInfo->m_szManufacturer);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "Device ChipType                 --- %s",
                 pDisplayInfo->m_szChipType);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "Approx. Total Device Memory     --- %s",
                 pDisplayInfo->m_szDisplayMemoryEnglish);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "Current Display Mode            --- %s",
                 pDisplayInfo->m_szDisplayModeEnglish);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "Device Driver Name              --- %s",
                 pDisplayInfo->m_szDriverName);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "Device Driver Version           --- %s",
                 pDisplayInfo->m_szDriverVersion);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "DirectDraw Acceleration Enabled --- %s",
                 pDisplayInfo->m_szDescription ? "Enabled" : "Disabled");
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "bNoHardware                     --- %s",
                 pDisplayInfo->m_bNoHardware ? "Enabled" : "Disabled");
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "b3DAccelerationExists Enabled   --- %s",
                 pDisplayInfo->m_b3DAccelerationExists ? "Enabled" : "Disabled");
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "b3DAccelerationEnabled Enabled  --- %s",
                 pDisplayInfo->m_b3DAccelerationEnabled ? "Enabled"
                         : "Disabled");
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "DDraw Status                    --- %s",
                 pDisplayInfo->m_szDDStatusEnglish);
    WEBRTC_TRACE(kTraceStateInfo, kTraceVideo, -1,
                 "D3D Status                      --- %s",
                 pDisplayInfo->m_szD3DStatusEnglish);

    
    OSVERSIONINFOEX osvie;
    osvie.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((LPOSVERSIONINFO) & osvie);
    










    
    if (osvie.dwMajorVersion < 6)
    {
        WEBRTC_TRACE(kTraceStateInfo, kTraceVideoRenderer, _id,
                     "Checking for RDP driver");
        if (_tcsncmp(pDisplayInfo->m_szDriverName, _T("RDPDD.dll"), 9) == 0)
        {
            
        }
    }

    if (pObject)
    {
        pObject->Release();
        pObject = NULL;
    }

    if (pContainer)
        pContainer->Release();

    if (m_pDxDiagProvider)
        m_pDxDiagProvider->Release();

    if (m_pDxDiagRoot)
        m_pDxDiagRoot->Release();

    if (pDisplayInfo)
        delete pDisplayInfo;

    if (coUninitializeIsRequired)
        CoUninitialize();

    return;
}





HRESULT VideoRenderWindowsImpl::GetStringValue(IDxDiagContainer* pObject,
                                               WCHAR* wstrName,
                                               TCHAR* strValue, int nStrLen)
{
    HRESULT hr;
    VARIANT var;
    VariantInit(&var);

    if (FAILED(hr = pObject->GetProp(wstrName, &var)))
        return hr;

    if (var.vt != VT_BSTR)
        return E_INVALIDARG;

#ifdef _UNICODE
    wcsncpy( strValue, var.bstrVal, nStrLen-1 );
#else
    wcstombs(strValue, var.bstrVal, nStrLen);
#endif
    strValue[nStrLen - 1] = TEXT('\0');
    VariantClear(&var);

    return S_OK;
}





HRESULT VideoRenderWindowsImpl::GetBoolValue(IDxDiagContainer* pObject,
                                             WCHAR* wstrName, BOOL* pbValue)
{
    HRESULT hr;
    VARIANT var;
    VariantInit(&var);

    if (FAILED(hr = pObject->GetProp(wstrName, &var)))
        return hr;

    if (var.vt != VT_BOOL)
        return E_INVALIDARG;

    *pbValue = (var.boolVal != 0);
    VariantClear(&var);

    return S_OK;
}

int VideoRenderWindowsImpl::CheckHWAcceleration()
{
    
    HKEY regKey;
    DWORD value = 0;
    DWORD valueLength = 4;

    bool directDraw = true;
    bool direct3D = true;
    bool dci = true;

    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\DirectDraw"),
                     0, KEY_QUERY_VALUE, &regKey) == ERROR_SUCCESS)
    {
        
        value = 0;
        if (RegQueryValueEx(regKey, _T("EmulationOnly"), NULL, NULL,
                            (BYTE*) &value, &valueLength) == ERROR_SUCCESS)
        {
            if (value == 1)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceVideo, -1,
                             "DirectDraw acceleration is disabled");
                directDraw = false;
            }
            else
            {
                WEBRTC_TRACE(kTraceWarning, kTraceVideo, -1,
                             "DirectDraw acceleration is enabled");
            }
        }
        else
        {
            
            WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                         "Could not find EmulationOnly key, DirectDraw acceleration is probably enabled");
        }
        RegCloseKey(regKey);
    }
    else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "Could not open DirectDraw settings");
    }

    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     _T("SOFTWARE\\Microsoft\\Direct3D\\Drivers"), 0,
                     KEY_QUERY_VALUE, &regKey) == ERROR_SUCCESS)
    {
        
        value = 0;
        if (RegQueryValueEx(regKey, _T("SoftwareOnly"), NULL, NULL,
                            (BYTE*) &value, &valueLength) == ERROR_SUCCESS)
        {
            if (value == 1)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceVideo, -1,
                             "Direct3D acceleration is disabled");
                direct3D = false;
            }
            else
            {
                WEBRTC_TRACE(kTraceWarning, kTraceVideo, -1,
                             "Direct3D acceleration is enabled");
            }
        }
        else
        {
            
            WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                         "Could not find SoftwarOnly key, Direct3D acceleration is probably enabled");
        }
        RegCloseKey(regKey);
    }
    else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "Could not open Direct3D settings");
    }

    
    if (RegOpenKeyEx(
                     HKEY_LOCAL_MACHINE,
                     _T(
                        "SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\DCI"),
                     0, KEY_QUERY_VALUE, &regKey) == ERROR_SUCCESS)
    {
        
        value = 0;
        if (RegQueryValueEx(regKey, _T("Timeout"), NULL, NULL, (BYTE*) &value,
                            &valueLength) == ERROR_SUCCESS)
        {
            if (value == 0)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceVideo, -1,
                             "DCI - DirectDraw acceleration is disabled");
                dci = false;
            }
            else if (value == 7)
            {
                WEBRTC_TRACE(kTraceWarning, kTraceVideo, -1,
                             "DCI is fully enabled");
            }
            else
            {
                WEBRTC_TRACE(
                             kTraceWarning,
                             kTraceVideo,
                             -1,
                             "DCI - DirectDraw acceleration is enabled, but short timeout: %d",
                             value);
            }
        }
        else
        {
            
            WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                         "Could not find Timeout key");
        }
        RegCloseKey(regKey);
    }
    else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "Could not open DCI settings");
    }

    
    if (dci == false || directDraw == false)
    {
        return -1;
    }

    return 0;
}

void VideoRenderWindowsImpl::CheckHWDriver(bool& badDriver,
                                           bool& fullAccelerationEnabled)
{
    
    HKEY regKey;
    DWORD value = 0;

    
    badDriver = false;
    fullAccelerationEnabled = true;

    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\VIDEO"), 0,
                     KEY_QUERY_VALUE, &regKey) == ERROR_SUCCESS)
    {
        
        value = 0;
        DWORD driverPathLen = 512;
        TCHAR driverPath[512];
        memset(driverPath, 0, driverPathLen * sizeof(TCHAR));

        long retVal = RegQueryValueEx(regKey, _T("\\Device\\Video0"), NULL,
                                      NULL, (BYTE*) driverPath, &driverPathLen);

        
        RegCloseKey(regKey);

        if (retVal == ERROR_SUCCESS)
        {
            

            
            
            TCHAR trueDriverPath[512];
            memset(trueDriverPath, 0, 512 * sizeof(TCHAR));

            
            

            
            int idx = 0;
            for (DWORD i = 18; i < (driverPathLen / sizeof(TCHAR)); i++)
            {
                trueDriverPath[idx++] = driverPath[i];
                if (driverPath[i] == _T('\\'))
                {
                    trueDriverPath[idx++] = driverPath[i];
                }
            }

            
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, trueDriverPath, 0,
                             KEY_QUERY_VALUE, &regKey) == ERROR_SUCCESS)
            {
                TCHAR driverName[64];
                memset(driverName, 0, 64 * sizeof(TCHAR));
                DWORD driverNameLength = 64;
                retVal = RegQueryValueEx(regKey, _T("drv"), NULL, NULL,
                                         (BYTE*) driverName, &driverNameLength);
                if (retVal == ERROR_SUCCESS)
                {
                    WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                                 "Graphics card driver name: %s", driverName);
                }
                DWORD accLevel = 0;
                DWORD accLevelS = sizeof(accLevel);

                RegQueryValueEx(regKey, _T("Acceleration.Level"), NULL, NULL,
                                (LPBYTE) & accLevel, &accLevelS);
                
                if (accLevel != 0)
                {
                    
                    RegCloseKey(regKey);

                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, trueDriverPath, 0,
                                     KEY_SET_VALUE, &regKey) == ERROR_SUCCESS)
                    {
                        
                        accLevel = 0;
                        LONG retVal;
                        retVal = RegSetValueEx(regKey,
                                               _T("Acceleration.Level"), NULL,
                                               REG_DWORD, (PBYTE) & accLevel,
                                               sizeof(DWORD));
                        if (retVal != ERROR_SUCCESS)
                        {
                            fullAccelerationEnabled = false;
                        }
                        else
                        {
                            RegQueryValueEx(regKey, _T("Acceleration.Level"),
                                            NULL, NULL, (LPBYTE) & accLevel,
                                            &accLevelS);
                            if (accLevel != 0)
                            {
                                fullAccelerationEnabled = false;
                            }
                            else
                            {
                                fullAccelerationEnabled = true;
                            }
                        }
                    }
                    else
                    {
                        fullAccelerationEnabled = false;
                    }
                }
                else
                {
                    fullAccelerationEnabled = true;
                }

                
                RegCloseKey(regKey);
            }
        }
    }
}

} 

