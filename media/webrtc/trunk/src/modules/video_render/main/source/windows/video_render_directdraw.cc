









#include "video_render_directdraw.h"
#include "video_render_windows_impl.h"
#include "Windows.h"
#include <ddraw.h>
#include <assert.h>
#include <initguid.h>
#include <MMSystem.h> 
DEFINE_GUID( IID_IDirectDraw7,0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b );

#include "thread_wrapper.h"
#include "event_wrapper.h"
#include "trace.h"
#include "critical_section_wrapper.h"



#include "module_common_types.h"

#pragma warning(disable: 4355) // 'this' : used in base member initializer list








namespace webrtc {

#define EXTRACT_BITS_RL(the_val, bits_start, bits_len) ((the_val >> (bits_start - 1)) & ((1 << bits_len) - 1)) 

WindowsThreadCpuUsage::WindowsThreadCpuUsage() :
    _lastGetCpuUsageTime(0),
    _lastCpuUsageTime(0),
    _hThread(::GetCurrentThread()),
    _cores(0),
    _lastCpuUsage(0)
{

    DWORD_PTR pmask, smask;
    DWORD access = PROCESS_QUERY_INFORMATION;
    if (GetProcessAffinityMask(
                               OpenProcess(access, false, GetCurrentProcessId()),
                               &pmask, &smask) != 0)
    {

        for (int i = 1; i < 33; i++)
        {
            if (EXTRACT_BITS_RL(pmask,i,1) == 0)
            {
                break;
            }
            _cores++;
        }
        
        if (_cores > 32)
        {
            _cores = 32;
        }
        if (_cores < 1)
        {
            _cores = 1;
        }
    }
    else
    {
        _cores = 1;
    }
    GetCpuUsage();
}


int WindowsThreadCpuUsage::GetCpuUsage()
{
    DWORD now = timeGetTime();

    _int64 newTime = 0;
    FILETIME creationTime;
    FILETIME exitTime;
    _int64 kernelTime = 0;
    _int64 userTime = 0;
    if (GetThreadTimes(_hThread, (FILETIME*) &creationTime, &exitTime,
                       (FILETIME*) &kernelTime, (FILETIME*) &userTime) != 0)
    {
        newTime = (kernelTime + userTime);
    }
    if (newTime == 0)
    {
        _lastGetCpuUsageTime = now;
        return _lastCpuUsage;
    }

    
    const DWORD diffTime = (now - _lastGetCpuUsageTime);
    _lastGetCpuUsageTime = now;

    if (newTime < _lastCpuUsageTime)
    {
        _lastCpuUsageTime = newTime;
        return _lastCpuUsage;
    }
    const int cpuDiff = (int) (newTime - _lastCpuUsageTime) / 10000;
    _lastCpuUsageTime = newTime;

    

    _lastCpuUsage = (int) (float((cpuDiff * 100)) / (diffTime * _cores) + 0.5f);

    if (_lastCpuUsage > 100)
    {
        _lastCpuUsage = 100;
    }
    return _lastCpuUsage;

}

DirectDrawStreamSettings::DirectDrawStreamSettings() :
    _startWidth(0.0F),
    _stopWidth(1.0F),
    _startHeight(0.0F),
    _stopHeight(1.0F),
    _cropStartWidth(0.0F),
    _cropStopWidth(1.0F),
    _cropStartHeight(0.0F),
    _cropStopHeight(1.0F)
{
}
;

DirectDrawBitmapSettings::DirectDrawBitmapSettings() :
    _transparentBitMap(NULL),
    _transparentBitmapLeft(0.0f),
    _transparentBitmapRight(1.0f),
    _transparentBitmapTop(0.0f),
    _transparentBitmapBottom(1.0f),
    _transparentBitmapWidth(0),
    _transparentBitmapHeight(0),
    _transparentBitmapColorKey(NULL),
    _transparentBitmapSurface(NULL)
{
}
;

DirectDrawBitmapSettings::~DirectDrawBitmapSettings()
{
    if (_transparentBitmapColorKey)
    {
        delete _transparentBitmapColorKey;
    }
    if (_transparentBitmapSurface)
    {
        _transparentBitmapSurface->Release();
    }
    _transparentBitmapColorKey = NULL;
    _transparentBitmapSurface = NULL;
}
;

int DirectDrawBitmapSettings::SetBitmap(Trace* _trace,
                                            DirectDraw* directDraw)
{
    VideoFrame tempVideoBuffer;
    HGDIOBJ oldhand;
    BITMAPINFO pbi;
    BITMAP bmap;
    HDC hdcNew;

    hdcNew = CreateCompatibleDC(0);

    
    GetObject(_transparentBitMap, sizeof(bmap), &bmap);

    
    oldhand = SelectObject(hdcNew, (HGDIOBJ) _transparentBitMap);

    
    DeleteObject(oldhand);

    pbi.bmiHeader.biSize = 40;
    pbi.bmiHeader.biWidth = bmap.bmWidth;
    pbi.bmiHeader.biHeight = bmap.bmHeight;
    pbi.bmiHeader.biPlanes = 1;
    pbi.bmiHeader.biBitCount = bmap.bmBitsPixel;
    pbi.bmiHeader.biCompression = BI_RGB;
    pbi.bmiHeader.biSizeImage = bmap.bmWidth * bmap.bmHeight * 3;

    tempVideoBuffer.VerifyAndAllocate(bmap.bmWidth * bmap.bmHeight * 4);

    
    
    int pixelHeight = GetDIBits(hdcNew, _transparentBitMap, 0, bmap.bmHeight,
                                tempVideoBuffer.Buffer(), &pbi, DIB_RGB_COLORS);
    if (pixelHeight == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw failed to GetDIBits in SetBitmap.");
        return -1;
        
    }

    DeleteDC(hdcNew);

    if (pbi.bmiHeader.biBitCount != 24 && pbi.bmiHeader.biBitCount != 32)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw failed to SetBitmap invalid bit depth");
        return -1;
        
    }

    DirectDrawSurfaceDesc ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
    ddsd.dwHeight = bmap.bmHeight;
    ddsd.dwWidth = bmap.bmWidth;

    ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;

    _transparentBitmapWidth = bmap.bmWidth;
    _transparentBitmapHeight = bmap.bmHeight;

    ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
    ddsd.ddpfPixelFormat.dwRBitMask = 0xff0000;
    ddsd.ddpfPixelFormat.dwGBitMask = 0xff00;
    ddsd.ddpfPixelFormat.dwBBitMask = 0xff;
    ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;

    if (_transparentBitmapSurface)
    {
        _transparentBitmapSurface->Release();
        _transparentBitmapSurface = NULL;
    }

    HRESULT ddrval =
            directDraw->CreateSurface(&ddsd, &_transparentBitmapSurface, NULL);
    if (FAILED(ddrval))
    {
        WEBRTC_TRACE(
                     kTraceError,
                     kTraceVideo,
                     -1,
                     "DirectDraw failed to CreateSurface _transparentBitmapSurface: 0x%x",
                     ddrval);
        return -1;
        
    }

    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddrval = _transparentBitmapSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
    if (ddrval == DDERR_SURFACELOST)
    {
        ddrval = _transparentBitmapSurface->Restore();
        if (ddrval != DD_OK)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVideo, -1,
                         "DirectDraw failed to restore lost _transparentBitmapSurface");
            return -1;
            
        }
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw restored lost _transparentBitmapSurface");

        ddrval
                = _transparentBitmapSurface->Lock(NULL, &ddsd, DDLOCK_WAIT,
                                                  NULL);
        if (ddrval != DD_OK)
        {
            WEBRTC_TRACE(
                         kTraceInfo,
                         kTraceVideo,
                         -1,
                         "DirectDraw lock error 0x%x _transparentBitmapSurface",
                         ddrval);
            return -1;
            
        }
    }
    unsigned char* dstPtr = (unsigned char*) ddsd.lpSurface;
    unsigned char* srcPtr = (unsigned char*) tempVideoBuffer.Buffer();

    int pitch = bmap.bmWidth * 4;
    if (ddsd.dwFlags & DDSD_PITCH)
    {
        pitch = ddsd.lPitch;
    }

    if (pbi.bmiHeader.biBitCount == 24)
    {
        ConvertRGB24ToARGB(srcPtr, dstPtr, bmap.bmWidth, bmap.bmHeight,
                                   0);
    }
    else
    {
        srcPtr += (bmap.bmWidth * 4) * (bmap.bmHeight - 1);

        for (int i = 0; i < bmap.bmHeight; ++i)
        {
            memcpy(dstPtr, srcPtr, bmap.bmWidth * 4);
            srcPtr -= bmap.bmWidth * 4;
            dstPtr += pitch;
        }
    }

    _transparentBitmapSurface->Unlock(NULL);
    return 0;
}





DirectDrawTextSettings::DirectDrawTextSettings() :
    _ptrText(NULL),
    _textLength(0),
    _colorRefText(RGB(255, 255, 255)), 
    _colorRefBackground(RGB(0, 0, 0)), 
    _textLeft(0.0f),
    _textRight(0.0f),
    _textTop(0.0f),
    _textBottom(0.0f),
    _transparent(true)
{
}

DirectDrawTextSettings::~DirectDrawTextSettings()
{
    if (_ptrText)
    {
        delete[] _ptrText;
    }
}

int DirectDrawTextSettings::SetText(const char* text, int textLength,
                                        COLORREF colorText, COLORREF colorBg,
                                        float left, float top, float right,
                                        float bottom)
{
    if (_ptrText)
    {
        delete[] _ptrText;
    }
    _ptrText = new char[textLength];
    memcpy(_ptrText, text, textLength);
    _textLength = textLength;
    _colorRefText = colorText;
    _colorRefBackground = colorBg;
    
    _textLeft = left;
    _textRight = right;
    _textTop = top;
    _textBottom = bottom;
    return 0;
}









DirectDrawChannel::DirectDrawChannel(DirectDraw* directDraw,
                                             VideoType blitVideoType,
                                             VideoType incomingVideoType,
                                             VideoType screenVideoType,
                                             VideoRenderDirectDraw* owner) :

    _critSect(CriticalSectionWrapper::CreateCriticalSection()), _refCount(1),
            _width(0), _height(0), _numberOfStreams(0), _doubleBuffer(false),
            _directDraw(directDraw), _offScreenSurface(NULL),
            _offScreenSurfaceNext(NULL), _incomingVideoType(incomingVideoType),
            _blitVideoType(blitVideoType),
            _originalBlitVideoType(blitVideoType),
            _screenVideoType(screenVideoType), _deliverInScreenType(false),
            _owner(owner)
{
    _directDraw->AddRef();
}

DirectDrawChannel::~DirectDrawChannel()
{
    if (_directDraw)
    {
        _directDraw->Release();
    }
    if (_offScreenSurface)
    {
        _offScreenSurface->Release();
    }
    if (_offScreenSurfaceNext)
    {
        _offScreenSurfaceNext->Release();
    }
    std::map<unsigned long long, DirectDrawStreamSettings*>::iterator it =
            _streamIdToSettings.begin();
    while (it != _streamIdToSettings.end())
    {
        DirectDrawStreamSettings* streamSettings = it->second;
        if (streamSettings)
        {
            delete streamSettings;
        }
        it = _streamIdToSettings.erase(it);
    }
    delete _critSect;
}

void DirectDrawChannel::AddRef()
{
    CriticalSectionScoped cs(_critSect);
    _refCount++;
}

void DirectDrawChannel::Release()
{
    bool deleteObj = false;
    _critSect->Enter();
    _refCount--;
    if (_refCount == 0)
    {
        deleteObj = true;
    }
    _critSect->Leave();

    if (deleteObj)
    {
        delete this;
    }
}

void DirectDrawChannel::SetStreamSettings(VideoRenderDirectDraw* DDobj,
                                              short streamId, float startWidth,
                                              float startHeight,
                                              float stopWidth, float stopHeight)
{
    
    unsigned long long lookupID = reinterpret_cast<unsigned long long> (DDobj);
    lookupID &= 0xffffffffffffffe0;
    lookupID <<= 11;
    lookupID += streamId;

    CriticalSectionScoped cs(_critSect);

    DirectDrawStreamSettings* streamSettings = NULL;

    std::map<unsigned long long, DirectDrawStreamSettings*>::iterator it =
            _streamIdToSettings.find(lookupID);
    if (it == _streamIdToSettings.end())
    {
        streamSettings = new DirectDrawStreamSettings();
        _streamIdToSettings[lookupID] = streamSettings;
    }
    else
    {
        streamSettings = it->second;
    }

    streamSettings->_startHeight = startHeight;
    streamSettings->_startWidth = startWidth;
    streamSettings->_stopWidth = stopWidth;
    streamSettings->_stopHeight = stopHeight;

    _offScreenSurfaceUpdated = false;
}

void DirectDrawChannel::SetStreamCropSettings(VideoRenderDirectDraw* DDObj,
                                                  short streamId,
                                                  float startWidth,
                                                  float startHeight,
                                                  float stopWidth,
                                                  float stopHeight)
{
    unsigned long long lookupID = reinterpret_cast<unsigned long long> (DDObj);
    lookupID &= 0xffffffffffffffe0;
    lookupID <<= 11;
    lookupID += streamId;

    CriticalSectionScoped cs(_critSect);

    DirectDrawStreamSettings* streamSettings = NULL;
    std::map<unsigned long long, DirectDrawStreamSettings*>::iterator it =
            _streamIdToSettings.find(lookupID);
    if (it == _streamIdToSettings.end())
    {
        streamSettings = new DirectDrawStreamSettings();
        _streamIdToSettings[streamId] = streamSettings;
    }
    else
    {
        streamSettings = it->second;
    }
    streamSettings->_cropStartWidth = startWidth;
    streamSettings->_cropStopWidth = stopWidth;
    streamSettings->_cropStartHeight = startHeight;
    streamSettings->_cropStopHeight = stopHeight;
}

int DirectDrawChannel::GetStreamSettings(VideoRenderDirectDraw* DDObj,
                                             short streamId, float& startWidth,
                                             float& startHeight,
                                             float& stopWidth,
                                             float& stopHeight)
{
    CriticalSectionScoped cs(_critSect);

    unsigned long long lookupID = reinterpret_cast<unsigned long long> (DDObj);
    lookupID &= 0xffffffffffffffe0;
    lookupID <<= 11;
    lookupID += streamId;

    DirectDrawStreamSettings* streamSettings = NULL;
    std::map<unsigned long long, DirectDrawStreamSettings*>::iterator it =
            _streamIdToSettings.find(lookupID);
    if (it == _streamIdToSettings.end())
    {
        
        return -1;
    }
    streamSettings = it->second;
    startWidth = streamSettings->_startWidth;
    startHeight = streamSettings->_startHeight;
    stopWidth = streamSettings->_stopWidth;
    stopHeight = streamSettings->_stopHeight;

    return 0;
}

bool DirectDrawChannel::IsOffScreenSurfaceUpdated(VideoRenderDirectDraw* DDobj)
{
    CriticalSectionScoped cs(_critSect);
    return _offScreenSurfaceUpdated;
}

void DirectDrawChannel::GetLargestSize(RECT* mixingRect)
{
    CriticalSectionScoped cs(_critSect);
    if (mixingRect)
    {
        if (mixingRect->bottom < _height)
        {
            mixingRect->bottom = _height;
        }
        if (mixingRect->right < _width)
        {
            mixingRect->right = _width;
        }
    }
}

int DirectDrawChannel::ChangeDeliverColorFormat(bool useScreenType)
{
    _deliverInScreenType = useScreenType;
    return FrameSizeChange(0, 0, 0);
}

WebRtc_Word32 DirectDrawChannel::RenderFrame(const WebRtc_UWord32 streamId,
                                                 VideoFrame& videoFrame)
{
    CriticalSectionScoped cs(_critSect);
    if (_width != videoFrame.Width() || _height != videoFrame.Height())
    {
        if (FrameSizeChange(videoFrame.Width(), videoFrame.Height(), 1) == -1)
        {
            return -1;
        }
    }
    return DeliverFrame(videoFrame.Buffer(), videoFrame.Length(),
                        videoFrame.TimeStamp());
}

int DirectDrawChannel::FrameSizeChange(int width, int height,
                                           int numberOfStreams)
{
    CriticalSectionScoped cs(_critSect);

    if (_directDraw == NULL)
    {
        return -1; 
    }
    if (_width == width && _height == height && _offScreenSurface
            && _offScreenSurfaceNext)
    {
        _numberOfStreams = numberOfStreams;
        return 0;
    }
    if (_offScreenSurface)
    {
        _offScreenSurface->Release();
        _offScreenSurface = NULL;
    }
    if (_offScreenSurfaceNext)
    {
        _offScreenSurfaceNext->Release();
        _offScreenSurfaceNext = NULL;
    }
    if (width && height)
    {
        _width = width;
        _height = height;
        _numberOfStreams = numberOfStreams;
    }

    
    DirectDrawSurfaceDesc ddsd;
    HRESULT ddrval = DD_OK;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
    ddsd.dwHeight = _height;
    ddsd.dwWidth = _width;
    




    
    if (_deliverInScreenType && _screenVideoType != kUnknown)
    {
        
        
        
        
        
        
    }
    else
    {
        WEBRTC_TRACE(
                     kTraceInfo,
                     kTraceVideo,
                     -1,
                     "DirectDrawChannel changing to originial blit video type %d",
                     _originalBlitVideoType);
        _blitVideoType = _originalBlitVideoType;
    }

    WEBRTC_TRACE(
                 kTraceInfo,
                 kTraceVideo,
                 -1,
                 "DirectDrawChannel::FrameSizeChange height %d, width %d, _blitVideoType %d",
                 ddsd.dwHeight, ddsd.dwWidth, _blitVideoType);
    switch (_blitVideoType)
    {
        case kYV12:
        {
            ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd.ddpfPixelFormat.dwFourCC = MAKEFOURCC('Y', 'V', '1', '2');
        }
            break;
        case kYUY2:
        {
            ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd.ddpfPixelFormat.dwFourCC = MAKEFOURCC('Y', 'U', 'Y', '2');
        }
            break;
        case kUYVY:
        {
            ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd.ddpfPixelFormat.dwFourCC = MAKEFOURCC('U', 'Y', 'V', 'Y');
        }
            break;
        case kIYUV:
        {
            ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd.ddpfPixelFormat.dwFourCC = MAKEFOURCC('I', 'Y', 'U', 'V');
        }
            break;
        case kARGB:
        {
            ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
            ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
            ddsd.ddpfPixelFormat.dwRBitMask = 0xff0000;
            ddsd.ddpfPixelFormat.dwGBitMask = 0xff00;
            ddsd.ddpfPixelFormat.dwBBitMask = 0xff;
            ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;
        }
            break;
        case kRGB24:
        {
            ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
            ddsd.ddpfPixelFormat.dwRGBBitCount = 24;
            ddsd.ddpfPixelFormat.dwRBitMask = 0xff0000;
            ddsd.ddpfPixelFormat.dwGBitMask = 0xff00;
            ddsd.ddpfPixelFormat.dwBBitMask = 0xff;
            ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;
        }
            break;
        case kRGB565:
        {
            ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
            ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
            ddsd.ddpfPixelFormat.dwRBitMask = 0x0000F800;
            ddsd.ddpfPixelFormat.dwGBitMask = 0x000007e0;
            ddsd.ddpfPixelFormat.dwBBitMask = 0x0000001F;
            ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;
        }
            break;
        case kARGB4444:
        {
            ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
            ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
            ddsd.ddpfPixelFormat.dwRBitMask = 0x00000f00;
            ddsd.ddpfPixelFormat.dwGBitMask = 0x000000f0;
            ddsd.ddpfPixelFormat.dwBBitMask = 0x0000000f;
            ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;
            break;
        }
        case kARGB1555:
        {
            ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
            ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
            ddsd.ddpfPixelFormat.dwRBitMask = 0x00007C00;
            ddsd.ddpfPixelFormat.dwGBitMask = 0x3E0;
            ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
            ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;
            break;
        }
        case kI420:
        {
            ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
            ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd.ddpfPixelFormat.dwFourCC = MAKEFOURCC('I', '4', '2', '0');
        }
            break;
        default:
            ddrval = S_FALSE;
    }

    if (ddrval == DD_OK)
    {
        if (!_owner->IsPrimaryOrMixingSurfaceOnSystem())
        {
            ddrval
                    = _directDraw->CreateSurface(&ddsd, &_offScreenSurface,
                                                 NULL);
            if (FAILED(ddrval))
            {
                WEBRTC_TRACE(
                             kTraceInfo,
                             kTraceVideo,
                             -1,
                             "CreateSurface failed for _offScreenSurface on VideoMemory, trying on System Memory");

                memset(&ddsd, 0, sizeof(ddsd));
                ddsd.dwSize = sizeof(ddsd);
                ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;

                ddsd.dwHeight = _height;
                ddsd.dwWidth = _width;

                ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
                _blitVideoType = kARGB;

                ddrval = _directDraw->CreateSurface(&ddsd, &_offScreenSurface,
                                                    NULL);
                if (FAILED(ddrval))
                {
                    WEBRTC_TRACE(
                                 kTraceError,
                                 kTraceVideo,
                                 -1,
                                 "DirectDraw failed to CreateSurface _offScreenSurface using SystemMemory: 0x%x",
                                 ddrval);
                }
                ddrval = _directDraw->CreateSurface(&ddsd,
                                                    &_offScreenSurfaceNext,
                                                    NULL);
                if (FAILED(ddrval))
                {
                    WEBRTC_TRACE(
                                 kTraceError,
                                 kTraceVideo,
                                 -1,
                                 "DirectDraw failed to CreateSurface _offScreenSurfaceNext using SystemMemory: 0x%x",
                                 ddrval);
                }
            }
            else
            {
                ddrval = _directDraw->CreateSurface(&ddsd,
                                                    &_offScreenSurfaceNext,
                                                    NULL);
                if (ddrval == DDERR_OUTOFVIDEOMEMORY)
                {
                    WEBRTC_TRACE(
                                 kTraceInfo,
                                 kTraceVideo,
                                 -1,
                                 "CreateSurface failed for _offScreenSurfaceNext on VideoMemory, trying on System Memory");

                    memset(&ddsd, 0, sizeof(ddsd));
                    ddsd.dwSize = sizeof(ddsd);
                    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;

                    ddsd.dwHeight = _height;
                    ddsd.dwWidth = _width;

                    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
                    _blitVideoType = kARGB;

                    ddrval = _directDraw->CreateSurface(&ddsd,
                                                        &_offScreenSurfaceNext,
                                                        NULL);
                    if (FAILED(ddrval))
                    {
                        WEBRTC_TRACE(
                                     kTraceError,
                                     kTraceVideo,
                                     -1,
                                     "DirectDraw failed to CreateSurface _offScreenSurfaceNext using SystemMemory: 0x%x",
                                     ddrval);
                    }
                }
            }
        }
        else
        {
            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;

            ddsd.dwHeight = _height;
            ddsd.dwWidth = _width;

            ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
            if (_owner->CanBltFourCC())
            {
                _blitVideoType = kARGB;
            }
            else
            {
                _blitVideoType = _originalBlitVideoType;
            }

            ddrval
                    = _directDraw->CreateSurface(&ddsd, &_offScreenSurface,
                                                 NULL);
            if (FAILED(ddrval))
            {
                WEBRTC_TRACE(
                             kTraceError,
                             kTraceVideo,
                             -1,
                             "DirectDraw failed to CreateSurface _offScreenSurface using SystemMemory: 0x%x",
                             ddrval);
            }

            ddrval = _directDraw->CreateSurface(&ddsd, &_offScreenSurfaceNext,
                                                NULL);
            if (FAILED(ddrval))
            {
                WEBRTC_TRACE(
                             kTraceError,
                             kTraceVideo,
                             -1,
                             "DirectDraw failed to CreateSurface _offScreenSurfaceNext using SystemMemory: 0x%x",
                             ddrval);
            }
        }
    }

    if (FAILED(ddrval))
    {
        
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw failed to CreateSurface : 0x%x", ddrval);
        return -1;
    }

    return 0;
}

int DirectDrawChannel::DeliverFrame(unsigned char* buffer, int bufferSize,
                                        unsigned int )
{
    CriticalSectionScoped cs(_critSect);

    if (CalcBufferSize(_incomingVideoType, _width, _height)
            != bufferSize)
    {
        
        return -1;
    }
    if (!_offScreenSurface || !_offScreenSurfaceNext)
    {
        if (_width && _height && _numberOfStreams)
        {
            
            FrameSizeChange(_width, _height, _numberOfStreams);
        }
        return -1;
    }
    if (_offScreenSurface->IsLost() == DDERR_SURFACELOST)
    {
        HRESULT ddrval = _offScreenSurface->Restore();
        if (ddrval != DD_OK)
        {
            
            _offScreenSurface->Release();
            _offScreenSurface = NULL;
            _offScreenSurfaceNext->Release();
            _offScreenSurfaceNext = NULL;
            return -1;
        }
        ddrval = _offScreenSurfaceNext->Restore();
        if (ddrval != DD_OK)
        {
            
            _offScreenSurface->Release();
            _offScreenSurface = NULL;
            _offScreenSurfaceNext->Release();
            _offScreenSurfaceNext = NULL;
            return -1;
        }
    }
    _doubleBuffer = false;

    
    DirectDrawSurface* offScreenSurface = _offScreenSurface;
    {

        if (_offScreenSurfaceUpdated)
        {
            
            offScreenSurface = _offScreenSurfaceNext;
            _doubleBuffer = true;
        }
    }

    DirectDrawSurfaceDesc ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    HRESULT ddrval = offScreenSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
    if (ddrval == DDERR_SURFACELOST)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDrawChannel::DeliverFrame offScreenSurface lost");
        ddrval = offScreenSurface->Restore();
        if (ddrval != DD_OK)
        {
            
            _offScreenSurface->Release();
            _offScreenSurface = NULL;
            _offScreenSurfaceNext->Release();
            _offScreenSurfaceNext = NULL;
            return -1;
        }
        return 0;
    }
    if (ddrval != DD_OK)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDrawChannel::DeliverFrame failed to lock");
        
        _offScreenSurface->Release();
        _offScreenSurface = NULL;
        _offScreenSurfaceNext->Release();
        _offScreenSurfaceNext = NULL;
        return -1;
    }

    int ret = 0;
    if (_incomingVideoType == kI420) {
      unsigned char* ptr = static_cast<unsigned char*>(ddsd.lpSurface);
      ret = ConvertFromI420(buffer, ddsd.lPitch, _blitVideoType, 0,
                            _width, _height, ptr);
    } else {
      assert(false &&
             "DirectDrawChannel::DeliverFrame wrong incoming video type");
             WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
             "%s wrong incoming video type:%d",
             __FUNCTION__, _incomingVideoType);
      ret = -1;
    }
    _offScreenSurfaceUpdated = true;
    offScreenSurface->Unlock(NULL);
    return ret;
}

int DirectDrawChannel::BlitFromOffscreenBufferToMixingBuffer(
                                                                 VideoRenderDirectDraw* DDobj,
                                                                 short streamID,
                                                                 DirectDrawSurface* mixingSurface,
                                                                 RECT &hwndRect,
                                                                 bool demuxing)
{
    HRESULT ddrval;
    RECT srcRect;
    RECT dstRect;
    DirectDrawStreamSettings* streamSettings = NULL;
    unsigned long long lookupID = reinterpret_cast<unsigned long long> (DDobj);
    lookupID &= 0xffffffffffffffe0;
    lookupID <<= 11;
    lookupID += streamID;

    CriticalSectionScoped cs(_critSect);

    if (_offScreenSurface == NULL)
    {
        
        return 0;
    }
    if (mixingSurface == NULL)
    {
        
        return 0;
    }

    std::map<unsigned long long, DirectDrawStreamSettings*>::iterator it =
            _streamIdToSettings.find(lookupID);
    if (it == _streamIdToSettings.end())
    {
        
        return 0;
    }
    streamSettings = it->second;

    int numberOfStreams = _numberOfStreams;
    if (!demuxing)
    {
        numberOfStreams = 1; 
    }

    switch (numberOfStreams)
    {
        case 0:
            return 0;
        case 1:
        {
            
            if (streamID > 0)
                return 0;

            ::SetRect(&srcRect, int(_width * streamSettings->_cropStartWidth),
                      int(_height * streamSettings->_cropStartHeight),
                      int(_width * streamSettings->_cropStopWidth), int(_height
                              * streamSettings->_cropStopHeight));

            ::SetRect(&dstRect, int(hwndRect.right
                    * streamSettings->_startWidth), int(hwndRect.bottom
                    * streamSettings->_startHeight), int(hwndRect.right
                    * streamSettings->_stopWidth), int(hwndRect.bottom
                    * streamSettings->_stopHeight));
        }
            break;
        case 2:
        case 3:
        case 4:
            
        {
            int width = _width >> 1;
            int height = _height >> 1;
            ::SetRect(&srcRect, int(width * streamSettings->_cropStartWidth),
                      int(height * streamSettings->_cropStartHeight), int(width
                              * streamSettings->_cropStopWidth), int(height
                              * streamSettings->_cropStopHeight));

            ::SetRect(&dstRect, int(hwndRect.right
                    * streamSettings->_startWidth), int(hwndRect.bottom
                    * streamSettings->_startHeight), int(hwndRect.right
                    * streamSettings->_stopWidth), int(hwndRect.bottom
                    * streamSettings->_stopHeight));

            
            if (streamID == 1)
            {
                ::OffsetRect(&srcRect, width, 0);
            }
            if (streamID == 2)
            {
                ::OffsetRect(&srcRect, 0, height);
            }
            if (streamID == 3)
            {
                ::OffsetRect(&srcRect, width, height);
            }
        }
            break;
        case 5:
        case 6:
        {
            const int width = (_width / (3 * 16)) * 16;
            const int widthMidCol = width + ((_width % (16 * 3)) / 16) * 16;
            const int height = _height / (2 * 16) * 16;
            if (streamID == 1 || streamID == 4)
            {
                ::SetRect(&srcRect, int(widthMidCol
                        * streamSettings->_cropStartWidth), int(height
                        * streamSettings->_cropStartHeight), int(widthMidCol
                        * streamSettings->_cropStopWidth), int(height
                        * streamSettings->_cropStopHeight));
            }
            else
            {
                ::SetRect(&srcRect,
                          int(width * streamSettings->_cropStartWidth),
                          int(height * streamSettings->_cropStartHeight),
                          int(width * streamSettings->_cropStopWidth),
                          int(height * streamSettings->_cropStopHeight));
            }
            ::SetRect(&dstRect, int(hwndRect.right
                    * streamSettings->_startWidth), int(hwndRect.bottom
                    * streamSettings->_startHeight), int(hwndRect.right
                    * streamSettings->_stopWidth), int(hwndRect.bottom
                    * streamSettings->_stopHeight));

            
            switch (streamID)
            {
                case 1:
                    ::OffsetRect(&srcRect, width, 0);
                    break;
                case 2:
                    ::OffsetRect(&srcRect, width + widthMidCol, 0);
                    break;
                case 3:
                    ::OffsetRect(&srcRect, 0, height);
                    break;
                case 4:
                    ::OffsetRect(&srcRect, width, height);
                    break;
                case 5:
                    ::OffsetRect(&srcRect, width + widthMidCol, height);
                    break;
            }
        }
            break;
        case 7:
        case 8:
        case 9:

        {
            const int width = (_width / (3 * 16)) * 16;
            const int widthMidCol = width + ((_width % (16 * 3)) / 16) * 16;
            const int height = _height / (3 * 16) * 16;
            const int heightMidRow = height + ((_height % (16 * 3)) / 16) * 16;

            ::SetRect(&dstRect, int(hwndRect.right
                    * streamSettings->_startWidth), int(hwndRect.bottom
                    * streamSettings->_startHeight), int(hwndRect.right
                    * streamSettings->_stopWidth), int(hwndRect.bottom
                    * streamSettings->_stopHeight));

            switch (streamID)
            {
                case 0:
                    
                    ::SetRect(&srcRect, int(width
                            * streamSettings->_cropStartWidth), int(height
                            * streamSettings->_cropStartHeight), int(width
                            * streamSettings->_cropStopWidth), int(height
                            * streamSettings->_cropStopHeight));
                    
                    ::OffsetRect(&srcRect, 0, 0);
                    break;
                case 1:
                    ::SetRect(
                              &srcRect,
                              int(widthMidCol * streamSettings->_cropStartWidth),
                              int(height * streamSettings->_cropStartHeight),
                              int(widthMidCol * streamSettings->_cropStopWidth),
                              int(height * streamSettings->_cropStopHeight));
                    ::OffsetRect(&srcRect, width, 0);
                    break;
                case 2:
                    ::SetRect(&srcRect, int(width
                            * streamSettings->_cropStartWidth), int(height
                            * streamSettings->_cropStartHeight), int(width
                            * streamSettings->_cropStopWidth), int(height
                            * streamSettings->_cropStopHeight));
                    ::OffsetRect(&srcRect, width + widthMidCol, 0);
                    break;
                case 3:
                    ::SetRect(&srcRect, int(width
                            * streamSettings->_cropStartWidth),
                              int(heightMidRow
                                      * streamSettings->_cropStartHeight),
                              int(width * streamSettings->_cropStopWidth),
                              int(heightMidRow
                                      * streamSettings->_cropStopHeight));
                    ::OffsetRect(&srcRect, 0, height);
                    break;
                case 4:
                    ::SetRect(
                              &srcRect,
                              int(widthMidCol * streamSettings->_cropStartWidth),
                              int(heightMidRow
                                      * streamSettings->_cropStartHeight),
                              int(widthMidCol * streamSettings->_cropStopWidth),
                              int(heightMidRow
                                      * streamSettings->_cropStopHeight));
                    ::OffsetRect(&srcRect, width, height);

                    break;
                case 5:
                    ::SetRect(&srcRect, int(width
                            * streamSettings->_cropStartWidth),
                              int(heightMidRow
                                      * streamSettings->_cropStartHeight),
                              int(width * streamSettings->_cropStopWidth),
                              int(heightMidRow
                                      * streamSettings->_cropStopHeight));
                    ::OffsetRect(&srcRect, width + widthMidCol, height);
                    break;
                case 6:
                    ::SetRect(&srcRect, int(width
                            * streamSettings->_cropStartWidth), int(height
                            * streamSettings->_cropStartHeight), int(width
                            * streamSettings->_cropStopWidth), int(height
                            * streamSettings->_cropStopHeight));
                    ::OffsetRect(&srcRect, 0, height + heightMidRow);
                    break;
                case 7:
                    ::SetRect(
                              &srcRect,
                              int(widthMidCol * streamSettings->_cropStartWidth),
                              int(height * streamSettings->_cropStartHeight),
                              int(widthMidCol * streamSettings->_cropStopWidth),
                              int(height * streamSettings->_cropStopHeight));
                    ::OffsetRect(&srcRect, width, height + heightMidRow);
                    break;
                case 8:
                    ::SetRect(&srcRect, int(width
                            * streamSettings->_cropStartWidth), int(height
                            * streamSettings->_cropStartHeight), int(width
                            * streamSettings->_cropStopWidth), int(height
                            * streamSettings->_cropStopHeight));
                    ::OffsetRect(&srcRect, width + widthMidCol, height
                            + heightMidRow);
                    break;
            }
        }
            break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        default:
        {
            ::SetRect(&srcRect, int(_width * streamSettings->_cropStartWidth),
                      int(_height * streamSettings->_cropStartHeight),
                      int(_width * streamSettings->_cropStopWidth), int(_height
                              * streamSettings->_cropStopHeight));

            ::SetRect(&dstRect, int(hwndRect.right
                    * streamSettings->_startWidth), int(hwndRect.bottom
                    * streamSettings->_startHeight), int(hwndRect.right
                    * streamSettings->_stopWidth), int(hwndRect.bottom
                    * streamSettings->_stopHeight));
        }
    }

    if (dstRect.right > hwndRect.right)
    {
        srcRect.right -= (int) ((float) (srcRect.right - srcRect.left)
                * ((float) (dstRect.right - hwndRect.right)
                        / (float) (dstRect.right - dstRect.left)));
        dstRect.right = hwndRect.right;
    }
    if (dstRect.left < hwndRect.left)
    {
        srcRect.left += (int) ((float) (srcRect.right - srcRect.left)
                * ((float) (hwndRect.left - dstRect.left)
                        / (float) (dstRect.right - dstRect.left)));
        dstRect.left = hwndRect.left;
    }
    if (dstRect.bottom > hwndRect.bottom)
    {
        srcRect.bottom -= (int) ((float) (srcRect.bottom - srcRect.top)
                * ((float) (dstRect.bottom - hwndRect.bottom)
                        / (float) (dstRect.bottom - dstRect.top)));
        dstRect.bottom = hwndRect.bottom;
    }
    if (dstRect.top < hwndRect.top)
    {
        srcRect.top += (int) ((float) (srcRect.bottom - srcRect.top)
                * ((float) (hwndRect.top - dstRect.top)
                        / (float) (dstRect.bottom - dstRect.top)));
        dstRect.top = hwndRect.top;
    }

    DDBLTFX ddbltfx;
    ZeroMemory(&ddbltfx, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwDDFX = DDBLTFX_NOTEARING;

    
    ddrval = mixingSurface->Blt(&dstRect, _offScreenSurface, &srcRect,
                                DDBLT_WAIT | DDBLT_DDFX, &ddbltfx);
    if (ddrval == DDERR_SURFACELOST)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "mixingSurface->Blt surface lost");
        ddrval = mixingSurface->Restore();
        if (ddrval != DD_OK)
        {
            
            return -1;
        }
    }
    else if (ddrval == DDERR_INVALIDRECT)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "mixingSurface->Blt DDERR_INVALIDRECT");
        WEBRTC_TRACE(
                     kTraceError,
                     kTraceVideo,
                     -1,
                     "dstRect co-ordinates - top: %d left: %d bottom: %d right: %d",
                     dstRect.top, dstRect.left, dstRect.bottom, dstRect.right);
        WEBRTC_TRACE(
                     kTraceError,
                     kTraceVideo,
                     -1,
                     "srcRect co-ordinates - top: %d left: %d bottom: %d right: %d",
                     srcRect.top, srcRect.left, srcRect.bottom, srcRect.right);

        
    }
    else if (ddrval != DD_OK)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "mixingSurface->Blt !DD_OK");
        WEBRTC_TRACE(
                     kTraceError,
                     kTraceVideo,
                     -1,
                     "DirectDraw blt mixingSurface BlitFromOffscreenBufferToMixingBuffer error 0x%x  ",
                     ddrval);

        
        WEBRTC_TRACE(
                     kTraceError,
                     kTraceVideo,
                     -1,
                     "dstRect co-ordinates - top: %d left: %d bottom: %d right: %d",
                     dstRect.top, dstRect.left, dstRect.bottom, dstRect.right);
        WEBRTC_TRACE(
                     kTraceError,
                     kTraceVideo,
                     -1,
                     "srcRect co-ordinates - top: %d left: %d bottom: %d right: %d",
                     srcRect.top, srcRect.left, srcRect.bottom, srcRect.right);

        









        
        return -1;
    }
    if (_doubleBuffer)
    {
        DirectDrawSurface* oldOffScreenSurface = _offScreenSurface;
        _offScreenSurface = _offScreenSurfaceNext;
        _offScreenSurfaceNext = oldOffScreenSurface;
        _doubleBuffer = false;
    }
    else
    {
        _offScreenSurfaceUpdated = false;
    }
    return 0;
}








VideoRenderDirectDraw::VideoRenderDirectDraw(Trace* trace,
                                                     HWND hWnd, bool fullscreen) :
            _trace(trace),
            _confCritSect(CriticalSectionWrapper::CreateCriticalSection()),
            _fullscreen(fullscreen),
            _demuxing(false),
            _transparentBackground(false),
            _supportTransparency(false),
            _canStretch(false),
            _canMirrorLeftRight(false),
            _clearMixingSurface(false),
            _deliverInScreenType(false),
            _renderModeWaitForCorrectScanLine(false),
            _deliverInHalfFrameRate(false),
            _deliverInQuarterFrameRate(false),
            _bCanBltFourcc(true),
            _frameChanged(false),
            _processCount(0),
            _hWnd(hWnd),
            _screenRect(),
            _mixingRect(),

            _incomingVideoType(kUnknown),
            _blitVideoType(kUnknown),
            _rgbVideoType(kUnknown),

            _directDraw(NULL),
            _primarySurface(NULL),
            _backSurface(NULL),
            _mixingSurface(NULL),
            _bitmapSettings(),
            _textSettings(),
            _directDrawChannels(),
            _directDrawZorder(),

            _fullScreenWaitEvent(EventWrapper::Create()),
            _screenEvent(EventWrapper::Create()),
            _screenRenderThread(
                                ThreadWrapper::CreateThread(
                                                            RemoteRenderingThreadProc,
                                                            this,
                                                            kRealtimePriority,
                                                            "Video_directdraw_thread")),
            _blit(true), _lastRenderModeCpuUsage(-1), _totalMemory(-1),
            _availableMemory(-1), _systemCPUUsage(0), _maxAllowedRenderTime(0),
            _nrOfTooLongRenderTimes(0),
            _isPrimaryOrMixingSurfaceOnSystem(false)
{
    SetRect(&_screenRect, 0, 0, 0, 0);
    SetRect(&_mixingRect, 0, 0, 0, 0);
    SetRect(&_originalHwndRect, 0, 0, 0, 0);
    ::GetClientRect(_hWnd, &_hwndRect);
}

VideoRenderDirectDraw::~VideoRenderDirectDraw()
{
    ThreadWrapper* temp = _screenRenderThread;
    _screenRenderThread = NULL;
    if (temp)
    {
        temp->SetNotAlive();
        _screenEvent->Set();
        _screenEvent->StopTimer();
        _fullScreenWaitEvent->StopTimer();

        if (temp->Stop())
        {
            delete temp;
        }
    }
    delete _screenEvent;
    delete _fullScreenWaitEvent;

    std::map<int, DirectDrawChannel*>::iterator it;
    it = _directDrawChannels.begin();
    while (it != _directDrawChannels.end())
    {
        it->second->Release();
        it = _directDrawChannels.erase(it);
    }
    if (_primarySurface)
    {
        _primarySurface->Release();
    }
    if (_mixingSurface)
    {
        _mixingSurface->Release();
    }

    std::map<unsigned char, DirectDrawBitmapSettings*>::iterator bitIt;

    bitIt = _bitmapSettings.begin();
    while (_bitmapSettings.end() != bitIt)
    {
        delete bitIt->second;
        bitIt = _bitmapSettings.erase(bitIt);
    }

    std::map<unsigned char, DirectDrawTextSettings*>::iterator textIt;
    textIt = _textSettings.begin();
    while (_textSettings.end() != textIt)
    {
        delete textIt->second;
        textIt = _textSettings.erase(textIt);
    }
    if (_directDraw)
    {
        _directDraw->Release();
        if (_fullscreen)
        {
            
            ::SetWindowPos(_hWnd, HWND_NOTOPMOST, _originalHwndRect.left,
                           _originalHwndRect.top, _originalHwndRect.right
                                   - _originalHwndRect.left,
                           _originalHwndRect.bottom - _originalHwndRect.top,
                           SWP_FRAMECHANGED);
            ::RedrawWindow(_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW
                    | RDW_ERASE);
            ::RedrawWindow(NULL, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW
                    | RDW_ERASE);
        }
    }
    delete _confCritSect;
}

WebRtc_Word32 VideoRenderDirectDraw::Init()
{
    int retVal = 0;
    HRESULT ddrval = DirectDrawCreateEx(NULL, (void**) &_directDraw,
                                        IID_IDirectDraw7, NULL);
    if (FAILED(ddrval) || NULL == _directDraw)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "Failed to created DirectDraw7 object");
        return -1;
        
    }
    retVal = CheckCapabilities();
    if (retVal != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw CheckCapabilities failed");
        return retVal;
    }
    if (_hWnd)
    {
        retVal = CreatePrimarySurface();
        if (retVal != 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                         "DirectDraw failed to CreatePrimarySurface");
            return retVal;
        }
        retVal = CreateMixingSurface();
        if (retVal != 0)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                         "DirectDraw failed to CreateMixingSurface");
            return retVal;
        }
        if (_screenRenderThread)
        {
            unsigned int tid;
            _screenRenderThread->Start(tid);
            WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                         "Screen Render thread started, thread id: %d", tid);
        }
        DWORD freq = 0;
        _directDraw->GetMonitorFrequency(&freq);
        if (freq == 0)
        {
            freq = 60;
        }
        
        _maxAllowedRenderTime = (int) (1000 / freq * 0.8F);
        _nrOfTooLongRenderTimes = 0;

        _screenEvent->StartTimer(true, 1000 / freq);

        _deliverInScreenType = false;
        _renderModeWaitForCorrectScanLine = false;
        _deliverInHalfFrameRate = false;
        _deliverInQuarterFrameRate = false;

        _lastRenderModeCpuUsage = -1;
        if (_fullscreen)
        {
            _fullScreenWaitEvent->StartTimer(true, 1);
        }

        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "Screen freq %d", freq);
    }
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                 "Created DirectDraw object");
    return 0;
}

WebRtc_Word32 VideoRenderDirectDraw::GetGraphicsMemory(
                                                           WebRtc_UWord64& totalMemory,
                                                           WebRtc_UWord64& availableMemory)
{
    CriticalSectionScoped cs(_confCritSect);

    if (_totalMemory == -1 || _availableMemory == -1)
    {
        totalMemory = 0;
        availableMemory = 0;
        return -1;
    }
    totalMemory = _totalMemory;
    availableMemory = _availableMemory;
    return 0;
}

int VideoRenderDirectDraw::GetScreenResolution(int& screenWidth,
                                                   int& screenHeight)
{
    CriticalSectionScoped cs(_confCritSect);

    screenWidth = _screenRect.right - _screenRect.left;
    screenHeight = _screenRect.bottom - _screenRect.top;
    return 0;
}

int VideoRenderDirectDraw::UpdateSystemCPUUsage(int systemCPU)
{
    CriticalSectionScoped cs(_confCritSect);
    if (systemCPU <= 100 && systemCPU >= 0)
    {
        _systemCPUUsage = systemCPU;
    }
    return 0;
}

int VideoRenderDirectDraw::CheckCapabilities()
{
    HRESULT ddrval = DD_OK;
    DDCAPS ddcaps;
    DDCAPS ddcapsEmul;
    memset(&ddcaps, 0, sizeof(ddcaps));
    memset(&ddcapsEmul, 0, sizeof(ddcapsEmul));
    ddcaps.dwSize = sizeof(ddcaps);
    ddcapsEmul.dwSize = sizeof(ddcapsEmul);
    if (_directDraw == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw object not created");
        return -1;
        
    }
    if (IsRectEmpty(&_screenRect))
    {
        ::GetWindowRect(GetDesktopWindow(), &_screenRect);
    }
    
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                 "ScreenRect. Top: %d, left: %d, bottom: %d, right: %d",
                 _screenRect.top, _screenRect.left, _screenRect.bottom,
                 _screenRect.right);

    bool fullAccelerationEnabled = false;
    bool badDriver = false;
    VideoRenderWindowsImpl::CheckHWDriver(badDriver, fullAccelerationEnabled);
    if (!fullAccelerationEnabled)
    {

        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "Direct draw Hardware acceleration is not enabled.");
        return -1;
        

    }

    
    
    ddrval = _directDraw->GetCaps(&ddcaps, &ddcapsEmul);
    if (ddrval != DD_OK)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw HW: could not get capabilities: %x", ddrval);
        return -1;
        
    }

    unsigned int minVideoMemory = 3 * 4 * (_screenRect.right
            * _screenRect.bottom); 

    
    _totalMemory = ddcaps.dwVidMemTotal;
    _availableMemory = ddcaps.dwVidMemFree;

    if (ddcaps.dwVidMemFree < minVideoMemory)
    {
        WEBRTC_TRACE(
                     kTraceError,
                     kTraceVideo,
                     -1,
                     "DirectDraw HW does not have enough memory, freeMem:%d, requiredMem:%d",
                     ddcaps.dwVidMemFree, minVideoMemory);
        
    }
    else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw video memory, freeMem:%d, totalMem:%d",
                     ddcaps.dwVidMemFree, ddcaps.dwVidMemTotal);
    }

    











    
    

    
    

    
    _supportTransparency = (ddcaps.dwCaps & DDCAPS_COLORKEY) ? 1 : 0;
    if (_supportTransparency)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw support colorkey");
    }
    else
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVideo, -1,
                     "DirectDraw don't support colorkey");
    }

    if (ddcaps.dwCaps2 & DDCAPS2_CANRENDERWINDOWED)
    {
        
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw support CANRENDERWINDOWED");
    }
    else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw don't support CANRENDERWINDOWED");
    }

    
    _canStretch = (ddcaps.dwCaps & DDCAPS_BLTSTRETCH) ? 1 : 0;
    if (_canStretch)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw blit can stretch");
    }
    else
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVideo, -1,
                     "DirectDraw blit can't stretch");
    }

    _canMirrorLeftRight = (ddcaps.dwFXAlphaCaps & DDBLTFX_MIRRORLEFTRIGHT) ? 1
            : 0;
    if (_canMirrorLeftRight)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw mirroring is supported");
    }
    else
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw mirroring is not supported");
    }

    
    _bCanBltFourcc = (ddcaps.dwCaps & DDCAPS_BLTFOURCC) ? 1 : 0;
    if (_bCanBltFourcc)
        _bCanBltFourcc = (ddcaps.dwCKeyCaps & DDCKEYCAPS_DESTBLT) ? 1 : 0;

    if (_bCanBltFourcc)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw can blit Fourcc");
        DWORD i_codes;
        ddrval = _directDraw->GetFourCCCodes(&i_codes, NULL);

        if (i_codes > 0)
        {
            DWORD* pi_codes = new DWORD[i_codes];

            ddrval = _directDraw->GetFourCCCodes(&i_codes, pi_codes);
            for (unsigned int i = 0; i < i_codes && _blitVideoType
                    != kI420; i++)
            {
                DWORD w = pi_codes[i];
                switch (w)
                {
                    case MAKEFOURCC('I', '4', '2', '0'):
                        
                        
                        WEBRTC_TRACE(kTraceInfo, kTraceVideo,
                                     -1, "DirectDraw support I420");
                        break;
                    case MAKEFOURCC('I', 'Y', 'U', 'V'): 
                    
                        
                        WEBRTC_TRACE(kTraceInfo, kTraceVideo,
                                     -1, "DirectDraw support IYUV");
                        break;
                    case MAKEFOURCC('U', 'Y', 'N', 'V'): 
                        WEBRTC_TRACE(kTraceInfo, kTraceVideo,
                                     -1, "DirectDraw support UYNV");
                        
                        break;
                    case MAKEFOURCC('Y', '4', '2', '2'): 
                        WEBRTC_TRACE(kTraceInfo, kTraceVideo,
                                     -1, "DirectDraw support Y422");
                        
                        break;
                    case MAKEFOURCC('Y', 'U', 'N', 'V'): 
                        WEBRTC_TRACE(kTraceInfo, kTraceVideo,
                                     -1, "DirectDraw support YUNV");
                        
                        break;
                    case MAKEFOURCC('Y', 'V', '1', '2'):
                        _blitVideoType = kYV12;
                        WEBRTC_TRACE(kTraceInfo, kTraceVideo,
                                     -1, "DirectDraw support YV12");
                        break;
                    case MAKEFOURCC('Y', 'U', 'Y', '2'):
                        if (_blitVideoType != kYV12)
                        {
                            _blitVideoType = kYUY2;
                        }
                        WEBRTC_TRACE(kTraceInfo, kTraceVideo,
                                     -1, "DirectDraw support YUY2");
                        break;
                    case MAKEFOURCC('U', 'Y', 'V', 'Y'):
                        if (_blitVideoType != kYV12)
                        {
                            _blitVideoType = kUYVY;
                        }
                        WEBRTC_TRACE(kTraceInfo, kTraceVideo,
                                     -1, "DirectDraw support UYVY");
                        break;
                    default:
                        WEBRTC_TRACE(kTraceInfo, kTraceVideo,
                                     -1, "DirectDraw unknown blit type %x", w);
                        break;
                }
            }
            delete[] pi_codes;
        }
    }
    return 0;
}

int VideoRenderDirectDraw::Stop()
{
    _confCritSect->Enter();

    _blit = false;

    _confCritSect->Leave();
    return 0;
}

bool VideoRenderDirectDraw::IsPrimaryOrMixingSurfaceOnSystem()
{
    return _isPrimaryOrMixingSurfaceOnSystem;
}

int VideoRenderDirectDraw::CreatePrimarySurface()
{
    
    DirectDrawSurfaceDesc ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    HRESULT ddrval = DD_OK;

    if (_directDraw == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw object not created");
        return -1;
        
    }
    if (_primarySurface)
    {
        _primarySurface->Release();
        _primarySurface = NULL;
    }

    if (!_fullscreen)
    {
        
        ddrval = _directDraw->SetCooperativeLevel(_hWnd, DDSCL_NORMAL);
        if (FAILED(ddrval))
        {
            
            WEBRTC_TRACE(kTraceWarning, kTraceVideo, -1,
                         "DirectDraw failed to set SetCooperativeLevel %x, ddrval");
        }
        
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY;

#ifndef NOGRAPHICSCARD_MEMORY
        ddrval = _directDraw->CreateSurface(&ddsd, &_primarySurface, NULL);
        if (FAILED(ddrval))
        {
            WEBRTC_TRACE(
                         kTraceError,
                         kTraceVideo,
                         -1,
                         "DirectDraw failed to CreateSurface _primarySurface using VideoMemory: 0x%x",
                         ddrval);
            WEBRTC_TRACE(
                         kTraceError,
                         kTraceVideo,
                         -1,
                         "\t HWND: 0x%x, top: %d, left: %d, bottom: %d, right: %d, dwFlags: %d. Line : %d",
                         _hWnd, _hwndRect.top, _hwndRect.left,
                         _hwndRect.bottom, _hwndRect.right, ddsd.dwFlags,
                         __LINE__);

#endif

            ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY;
            ddrval = _directDraw->CreateSurface(&ddsd, &_primarySurface, NULL);
            if (FAILED(ddrval))
            {
                WEBRTC_TRACE(
                             kTraceError,
                             kTraceVideo,
                             -1,
                             "DirectDraw failed to CreateSurface _primarySurface using SystemMemory: 0x%x",
                             ddrval);
                if (ddrval != 0x887600E1)
                {
                    _directDraw->Release();
                    _directDraw = 0;
                }
                return -1;
                
            }
            _isPrimaryOrMixingSurfaceOnSystem = true;
            WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                         "DirectDraw _primarySurface on SystemMemory");

#ifndef NOGRAPHICSCARD_MEMORY
        }
#endif

        
        LPDIRECTDRAWCLIPPER directDrawClipper;
        ddrval = _directDraw->CreateClipper(0, &directDrawClipper, NULL );
        if (ddrval != DD_OK)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                         "DirectDraw failed to CreateClipper");
            _primarySurface->Release();
            _directDraw->Release();
            _primarySurface = 0;
            _directDraw = 0;
            return -1;
            
        }
        
        
        ddrval = directDrawClipper->SetHWnd(0, _hWnd);
        if (ddrval != DD_OK)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                         "DirectDraw failed to SetHWnd");
            _primarySurface->Release();
            _directDraw->Release();
            _primarySurface = 0;
            _directDraw = 0;
            return -1;
            
        }
        
        ddrval = _primarySurface->SetClipper(directDrawClipper);
        directDrawClipper->Release(); 
        if (ddrval != DD_OK)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                         "DirectDraw failed to SetClipper");
            _primarySurface->Release();
            _directDraw->Release();
            _primarySurface = 0;
            _directDraw = 0;
            return -1;
            
        }
    }
    else
    {
        










        
        ::GetWindowRect(_hWnd, &_originalHwndRect);

        
        ddrval = _directDraw->SetCooperativeLevel(_hWnd, DDSCL_EXCLUSIVE
                | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT);

        if (FAILED(ddrval))
        {
            WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                         "DirectDraw failed to SetCooperativeLevel DDSCL_EXCLUSIVE");
            WEBRTC_TRACE(
                         kTraceError,
                         kTraceVideo,
                         -1,
                         "\t HWND: 0x%x, top: %d, left: %d, bottom: %d, right: %d, dwFlags: %d. Line : %d",
                         _hWnd, _hwndRect.top, _hwndRect.left,
                         _hwndRect.bottom, _hwndRect.right, ddsd.dwFlags,
                         __LINE__);

            _directDraw->Release();
            _directDraw = 0;
            return -1;
            
        }
        ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP
                | DDSCAPS_COMPLEX | DDSCAPS_VIDEOMEMORY;
        ddsd.dwBackBufferCount = 1;

        ddrval = _directDraw->CreateSurface(&ddsd, &_primarySurface, NULL);
        if (FAILED(ddrval))
        {
            WEBRTC_TRACE(
                         kTraceError,
                         kTraceVideo,
                         -1,
                         "DirectDraw failed to CreateSurface _primarySurface, fullscreen mode: 0x%x",
                         ddrval);
            WEBRTC_TRACE(
                         kTraceError,
                         kTraceVideo,
                         -1,
                         "\t HWND: 0x%x, top: %d, left: %d, bottom: %d, right: %d, dwFlags: %d. Line : %d",
                         _hWnd, _hwndRect.top, _hwndRect.left,
                         _hwndRect.bottom, _hwndRect.right, ddsd.dwFlags,
                         __LINE__);

            _directDraw->Release();
            _directDraw = 0;
            return -1;
            
        }
        
        DirectDrawCaps ddsCaps;
        ZeroMemory(&ddsCaps, sizeof(ddsCaps));
        ddsCaps.dwCaps = DDSCAPS_BACKBUFFER | DDSCAPS_VIDEOMEMORY;

        ddrval = _primarySurface->GetAttachedSurface(&ddsCaps, &_backSurface);
        if (FAILED(ddrval))
        {
            WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                         "DirectDraw failed to GetAttachedSurface, fullscreen mode ");
            WEBRTC_TRACE(
                         kTraceError,
                         kTraceVideo,
                         -1,
                         "\t HWND: 0x%x, top: %d, left: %d, bottom: %d, right: %d, dwFlags: %d. Line : %d",
                         _hWnd, _hwndRect.top, _hwndRect.left,
                         _hwndRect.bottom, _hwndRect.right, ddsd.dwFlags,
                         __LINE__);

            _primarySurface->Release();
            _directDraw->Release();
            _primarySurface = 0;
            _directDraw = 0;
            return -1;
            
        }
        
        ZeroMemory(&ddsd, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
    }

    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    
    ddrval = _primarySurface->GetSurfaceDesc(&ddsd);
    if (!(SUCCEEDED(ddrval) && (ddsd.dwFlags & DDSD_WIDTH) && (ddsd.dwFlags
            & DDSD_HEIGHT)))
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw failed to GetSurfaceDesc _primarySurface");
        WEBRTC_TRACE(
                     kTraceError,
                     kTraceVideo,
                     -1,
                     "\t HWND: 0x%x, top: %d, left: %d, bottom: %d, right: %d, dwFlags: %d. Line : %d",
                     _hWnd, _hwndRect.top, _hwndRect.left, _hwndRect.bottom,
                     _hwndRect.right, ddsd.dwFlags, __LINE__);

        _primarySurface->Release();
        _directDraw->Release();
        _primarySurface = 0;
        _directDraw = 0;
        return -1;
        
    }
    

    
    ::SetRect(&_screenRect, 0, 0, ddsd.dwWidth, ddsd.dwHeight);

    
    if (ddsd.ddpfPixelFormat.dwFlags & DDPF_RGB)
    {
        
        switch (ddsd.ddpfPixelFormat.dwRGBBitCount)
        {
            case 16:
                switch (ddsd.ddpfPixelFormat.dwGBitMask)
                {
                    case 0x00e0:
                        _rgbVideoType = kARGB4444;
                        break;
                    case 0x03e0:
                        _rgbVideoType = kARGB1555;
                        break;
                    case 0x07e0:
                        _rgbVideoType = kRGB565;
                        break;
                }
                break;
            case 24:
                _rgbVideoType = kRGB24;
                break;
            case 32:
                _rgbVideoType = kARGB;
                break;
        }
    }
    switch (_blitVideoType)
    {
        case kI420:
        case kIYUV:
        case kYUY2:
        case kYV12:
        case kUYVY:
            _incomingVideoType = kI420;
            break;
        case kUnknown:
            _blitVideoType = _rgbVideoType;
            _incomingVideoType = kI420;
            break;
        default:
            _blitVideoType = _rgbVideoType;
            _incomingVideoType = kI420;
            break;
    }
    WEBRTC_TRACE(
                 kTraceInfo,
                 kTraceVideo,
                 -1,
                 "DirectDraw created _primarySurface, _blitVideoType %d, _rgbvideoType %d",
                 _blitVideoType, _rgbVideoType);
    return 0;
}

int VideoRenderDirectDraw::CreateMixingSurface()
{
    if (_directDraw == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw object not created");
        return -1;
        
    }

    if (_fullscreen)
    {
        ::CopyRect(&_hwndRect, &_screenRect);
    }
    else
    {
        
        ::GetClientRect(_hWnd, &_hwndRect);
    }

    if (_mixingSurface)
    {
        _mixingSurface->Release();
        _mixingSurface = NULL;
    }
    
    DirectDrawSurfaceDesc ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
    ddsd.dwHeight = _hwndRect.bottom;
    ddsd.dwWidth = _hwndRect.right;

    




#ifndef NOGRAPHICSCARD_MEMORY
    HRESULT ddrval = _directDraw->CreateSurface(&ddsd, &_mixingSurface, NULL);
    if (FAILED(ddrval))
    {
        WEBRTC_TRACE(
                     kTraceError,
                     kTraceVideo,
                     -1,
                     "DirectDraw failed to CreateSurface _mixingSurface using VideoMemory: 0x%x",
                     ddrval);
        WEBRTC_TRACE(
                     kTraceError,
                     kTraceVideo,
                     -1,
                     "\t HWND: 0x%x, top: %d, left: %d, bottom: %d, right: %d, dwFlags: %d",
                     _hWnd, _hwndRect.top, _hwndRect.left, _hwndRect.bottom,
                     _hwndRect.right, ddsd.dwFlags);
#endif

        ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
        HRESULT ddrval = _directDraw->CreateSurface(&ddsd, &_mixingSurface,
                                                    NULL);
        if (FAILED(ddrval))
        {
            WEBRTC_TRACE(
                         kTraceError,
                         kTraceVideo,
                         -1,
                         "DirectDraw failed to CreateSurface _mixingSurface on System Memory: 0x%x",
                         ddrval);
            return -1;
            
        }
        _isPrimaryOrMixingSurfaceOnSystem = true;
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw CreateSurface _mixingSurface on SystemMemory");

#ifndef NOGRAPHICSCARD_MEMORY        
    }
#endif

    _clearMixingSurface = true;
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                 "DirectDraw _mixingSurface created");
    return 0;
}

VideoRenderCallback* VideoRenderDirectDraw::CreateChannel(WebRtc_UWord32 channel,
                                                                  WebRtc_UWord32 zOrder,
                                                                  float startWidth,
                                                                  float startHeight,
                                                                  float stopWidth,
                                                                  float stopHeight)
{
    if (!_canStretch)
    {
        if (startWidth != 0.0f || startHeight != 0.0f || stopWidth != 1.0f
                || stopHeight != 1.0f)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                         "DirectDraw failed to CreateChannel HW don't support stretch");
            return NULL;
        }
    }
    DirectDrawChannel* ddobj =
            new DirectDrawChannel(_directDraw, _blitVideoType,
                                      _incomingVideoType, _rgbVideoType, this);
    ddobj->SetStreamSettings(this, 0, startWidth, startHeight, stopWidth,
                             stopHeight);

    
    _directDrawChannels[channel & 0x0000ffff] = ddobj;

    
    
    _directDrawZorder.insert(ZorderPair(zOrder, channel & 0x0000ffff));
    return ddobj;
}

int VideoRenderDirectDraw::AddDirectDrawChannel(int channel,
                                                    unsigned char streamID,
                                                    int zOrder,
                                                    DirectDrawChannel* ddObj)
{
    
    streamID = 0;
    unsigned int streamChannel = (streamID << 16) + (channel & 0x0000ffff);

    
    _directDrawChannels[channel & 0x0000ffff] = ddObj;

    _demuxing = true; 

    
    _directDrawZorder.insert(ZorderPair(zOrder, streamChannel));
    return 0;
}

DirectDrawChannel* VideoRenderDirectDraw::ShareDirectDrawChannel(
                                                                         int channel)
{
    CriticalSectionScoped cs(_confCritSect);

    DirectDrawChannel* obj = NULL;

    std::map<int, DirectDrawChannel*>::iterator ddIt;
    ddIt = _directDrawChannels.find(channel & 0x0000ffff);
    if (ddIt != _directDrawChannels.end())
    {
        obj = ddIt->second;
        obj->AddRef();
    }
    return obj;
}

WebRtc_Word32 VideoRenderDirectDraw::DeleteChannel(const WebRtc_UWord32 channel)
{
    CriticalSectionScoped cs(_confCritSect);

    

    
    std::multimap<int, unsigned int>::iterator it;
    it = _directDrawZorder.begin();
    while (it != _directDrawZorder.end())
    {
        
        if ((channel & 0x0000ffff) == (it->second & 0x0000ffff))
        {
            it = _directDrawZorder.erase(it);
            break;
        }
        it++;
    }

    std::map<int, DirectDrawChannel*>::iterator ddIt;
    ddIt = _directDrawChannels.find(channel & 0x0000ffff);
    if (ddIt != _directDrawChannels.end())
    {
        ddIt->second->Release();
        _directDrawChannels.erase(ddIt);
        _clearMixingSurface = true;
    }

    return 0;
}

WebRtc_Word32 VideoRenderDirectDraw::GetStreamSettings(const WebRtc_UWord32 channel,
                                                           const WebRtc_UWord16 streamId,
                                                           WebRtc_UWord32& zOrder,
                                                           float& startWidth,
                                                           float& startHeight,
                                                           float& stopWidth,
                                                           float& stopHeight)
{
    CriticalSectionScoped cs(_confCritSect);

    std::map<int, DirectDrawChannel*>::iterator ddIt;
    ddIt = _directDrawChannels.find(channel & 0x0000ffff);
    if (ddIt == _directDrawChannels.end())
    {
        
        return -1;
    }

    DirectDrawChannel* ptrChannel = ddIt->second;
    
    
    if (ptrChannel->GetStreamSettings(this, 0, startWidth, startHeight,
                                      stopWidth, stopHeight) == -1)
    {
        
        return -1;
    }

    
    std::multimap<int, unsigned int>::iterator it;
    it = _directDrawZorder.begin();
    while (it != _directDrawZorder.end())
    {
        if ((channel & 0x0000ffff) == (it->second & 0x0000ffff))
        {
            
            zOrder = (unsigned int) (it->first);
            break;
        }
        it++;
    }

    return 0;
}

int VideoRenderDirectDraw::GetChannels(std::list<int>& channelList)
{
    CriticalSectionScoped cs(_confCritSect);

    std::map<int, DirectDrawChannel*>::iterator ddIt;
    ddIt = _directDrawChannels.begin();

    while (ddIt != _directDrawChannels.end())
    {
        int channel = ddIt->first;
        if (channel == 0x0000ffff)
        {
            channel = -1;
        }
        channelList.push_back(channel);
        ddIt++;
    }
    return 0;
}

bool VideoRenderDirectDraw::HasChannel(int channel)
{
    CriticalSectionScoped cs(_confCritSect);

    std::map<int, DirectDrawChannel*>::iterator ddIt;
    ddIt = _directDrawChannels.find(channel & 0x0000ffff);
    if (ddIt != _directDrawChannels.end())
    {
        return true;
    }
    return false;
}

bool VideoRenderDirectDraw::HasChannels()
{
    CriticalSectionScoped cs(_confCritSect);

    if (_directDrawChannels.begin() != _directDrawChannels.end())
    {
        return true;
    }
    return false;
}

bool VideoRenderDirectDraw::IsFullScreen()
{
    return _fullscreen;
}

VideoType VideoRenderDirectDraw::GetPerferedVideoFormat()
{
    return _incomingVideoType;
}


DirectDrawChannel* VideoRenderDirectDraw::ConfigureDirectDrawChannel(int channel,
                                                                             unsigned char streamID,
                                                                             int zOrder,
                                                                             float left,
                                                                             float top,
                                                                             float right,
                                                                             float bottom)
{
    
    streamID = 0;

    CriticalSectionScoped cs(_confCritSect);

    if (!_canStretch)
    {
        if (left != 0.0f || top != 0.0f || right != 1.0f || bottom != 1.0f)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                         "DirectDraw failed to ConfigureDirectDrawChannel HW don't support stretch");
            return NULL;
        }
    }
    std::map<int, DirectDrawChannel*>::iterator ddIt;
    ddIt = _directDrawChannels.find(channel & 0x0000ffff);
    DirectDrawChannel* ddobj = NULL;
    if (ddIt != _directDrawChannels.end())
    {
        ddobj = ddIt->second;
    }
    if (ddobj == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, -1,
                     "DirectDraw failed to find channel");
        return NULL;
    }
    unsigned int streamChannel = (streamID << 16) + (channel & 0x0000ffff);
    
    std::multimap<int, unsigned int>::iterator it;
    it = _directDrawZorder.begin();
    while (it != _directDrawZorder.end())
    {
        if (streamChannel == it->second)
        {
            it = _directDrawZorder.erase(it);
            break;
        }
        it++;
    }
    
    it = _directDrawZorder.begin();
    while (it != _directDrawZorder.end())
    {
        if (channel == (it->second & 0x0000ffff))
        {
            _demuxing = true;
            break;
        }
        it++;
    }
    if (it == _directDrawZorder.end())
    {
        _demuxing = false;
    }

    _clearMixingSurface = true;

    if (left == 0.0f && top == 0.0f && right == 0.0f && bottom == 0.0f)
    {
        
        _directDrawChannels.erase(ddIt);
        ddobj->Release();
        return NULL;
    }
    ddobj->SetStreamSettings(this, streamID, left, top, right, bottom);

    _directDrawZorder.insert(ZorderPair(zOrder, streamChannel));
    return ddobj;
}

WebRtc_Word32 VideoRenderDirectDraw::SetCropping(const WebRtc_UWord32 channel,
                                                     const WebRtc_UWord16 streamID,
                                                     float left, float top,
                                                     float right, float bottom)
{
    CriticalSectionScoped cs(_confCritSect);
    if (!_canStretch)
    {
        if (left != 0.0f || top != 0.0f || right != 1.0f || bottom != 1.0f)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, -1,
                         "DirectDraw failed to SetCropping HW don't support stretch");
            return -1;
            
        }
    }

    std::map<int, DirectDrawChannel*>::iterator ddIt;
    ddIt = _directDrawChannels.find(channel & 0x0000ffff);
    if (ddIt != _directDrawChannels.end())
    {
        DirectDrawChannel* ddobj = ddIt->second;
        if (ddobj)
        {
            
            ddobj->SetStreamCropSettings(this, 0, left, top, right, bottom);
            
        }
    }
    return 0;
}

WebRtc_Word32 VideoRenderDirectDraw::ConfigureRenderer(const WebRtc_UWord32 channel,
                                                           const WebRtc_UWord16 streamId,
                                                           const unsigned int zOrder,
                                                           const float left,
                                                           const float top,
                                                           const float right,
                                                           const float bottom)
{
    if (ConfigureDirectDrawChannel(channel, (unsigned char) streamId, zOrder,
                                   left, top, right, bottom) == NULL)
    {
        if (left == 0.0f && top == 0.0f && right == 0.0f && bottom == 0.0f)
        {
            WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, -1,
                         "ConfigureRender, removed channel:%d streamId:%d",
                         channel, streamId);
        }
        else
        {
            WEBRTC_TRACE(
                         kTraceError,
                         kTraceVideoRenderer,
                         -1,
                         "DirectDraw failed to ConfigureRenderer for channel: %d",
                         channel);
            return -1;
        }
    }
    return 0;
}


WebRtc_Word32 VideoRenderDirectDraw::SetText(const WebRtc_UWord8 textId,
                                                 const WebRtc_UWord8* text,
                                                 const WebRtc_Word32 textLength,
                                                 const WebRtc_UWord32 colorText,
                                                 const WebRtc_UWord32 colorBg,
                                                 const float left,
                                                 const float top,
                                                 const float right,
                                                 const float bottom)
{
    DirectDrawTextSettings* textSetting = NULL;

    CriticalSectionScoped cs(_confCritSect);

    _frameChanged = true;

    std::map<unsigned char, DirectDrawTextSettings*>::iterator it;
    it = _textSettings.find(textId);
    if (it != _textSettings.end())
    {
        if (it->second)
        {
            textSetting = it->second;
        }
    }
    _clearMixingSurface = true;

    if (text == NULL || textLength == 0)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw remove text textId:%d", textId);
        if (textSetting)
        {
            delete textSetting;
            _textSettings.erase(it);
        }
        return 0;
    }

    
    if (left > 1.0f || left < 0.0f)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw SetText invalid parameter");
        return -1;
        
    }
    if (top > 1.0f || top < 0.0f)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw SetText invalid parameter");
        return -1;
        
    }
    if (right > 1.0f || right < 0.0f)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw SetText invalid parameter");
        return -1;
        
    }
    if (bottom > 1.0f || bottom < 0.0f)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw SetText invalid parameter");
        return -1;
        
    }
    if (textSetting == NULL)
    {
        textSetting = new DirectDrawTextSettings();
    }
    int retVal = textSetting->SetText((const char*) text, textLength,
                                      (COLORREF) colorText, (COLORREF) colorBg,
                                      left, top, right, bottom);
    if (retVal != 0)
    {
        delete textSetting;
        textSetting = NULL;
        _textSettings.erase(textId);
        return retVal;
    }
    if (textSetting)
    {
        _textSettings[textId] = textSetting;
    }
    return retVal;
}


WebRtc_Word32 VideoRenderDirectDraw::SetBitmap(const void* bitMap,
                                                   const WebRtc_UWord8 pictureId,
                                                   const void* colorKey,
                                                   const float left,
                                                   const float top,
                                                   const float right,
                                                   const float bottom)
{
    DirectDrawBitmapSettings* bitmapSetting = NULL;

    CriticalSectionScoped cs(_confCritSect);

    _frameChanged = true;
    std::map<unsigned char, DirectDrawBitmapSettings*>::iterator it;
    it = _bitmapSettings.find(pictureId);
    if (it != _bitmapSettings.end())
    {
        if (it->second)
        {
            bitmapSetting = it->second;
        }
    }
    _clearMixingSurface = true;

    if (bitMap == NULL)
    {
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw remove bitmap pictureId:%d", pictureId);
        if (bitmapSetting)
        {
            delete bitmapSetting;
            _bitmapSettings.erase(it);
        }
        return 0;
    }

    
    if (left > 1.0f || left < 0.0f)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw SetBitmap invalid parameter");
        return -1;
        
    }
    if (top > 1.0f || top < 0.0f)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw SetBitmap invalid parameter");
        return -1;
        
    }
    if (right > 1.0f || right < 0.0f)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw SetBitmap invalid parameter");
        return -1;
        
    }
    if (bottom > 1.0f || bottom < 0.0f)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw SetBitmap invalid parameter");
        return -1;
        
    }
    if (!_canStretch)
    {
        if (left != 0.0f || top != 0.0f || right != 1.0f || bottom != 1.0f)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                         "DirectDraw failed to SetBitmap HW don't support stretch");
            return -1;
            
        }
    }
    if (bitmapSetting == NULL)
    {
        bitmapSetting = new DirectDrawBitmapSettings();
    }

    bitmapSetting->_transparentBitMap = (HBITMAP) bitMap;
    bitmapSetting->_transparentBitmapLeft = left;
    bitmapSetting->_transparentBitmapRight = right;
    bitmapSetting->_transparentBitmapTop = top;
    bitmapSetting->_transparentBitmapBottom = bottom;

    
    if (colorKey)
    {
        
        DDCOLORKEY* ddColorKey =
                static_cast<DDCOLORKEY*> (const_cast<void*> (colorKey));
        if (!_supportTransparency)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                         "DirectDraw failed to SetBitmap HW don't support transparency");
            return -1;
            
        }
        if (bitmapSetting->_transparentBitmapColorKey == NULL)
        {
            bitmapSetting->_transparentBitmapColorKey = new DDCOLORKEY();
        }

        if (ddColorKey)
        {
            bitmapSetting->_transparentBitmapColorKey->dwColorSpaceLowValue
                    = ddColorKey->dwColorSpaceLowValue;
            bitmapSetting->_transparentBitmapColorKey->dwColorSpaceHighValue
                    = ddColorKey->dwColorSpaceHighValue;
        }
    }
    int retval = bitmapSetting->SetBitmap(_trace, _directDraw);
    if (retval != 0)
    {
        delete bitmapSetting;
        bitmapSetting = NULL;
        _bitmapSettings.erase(pictureId);
        return retval;
    }
    if (bitmapSetting)
    {
        _bitmapSettings[pictureId] = bitmapSetting;
    }
    return retval;
}


WebRtc_Word32 VideoRenderDirectDraw::SetTransparentBackground(
                                                                  const bool enable)
{
    CriticalSectionScoped cs(_confCritSect);

    if (_supportTransparency)
    {
        _transparentBackground = enable;
        if (enable)
        {
            WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                         "DirectDraw enabled TransparentBackground");
        }
        else
        {
            WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                         "DirectDraw disabled TransparentBackground");
        }
        return 0;
    }
    WEBRTC_TRACE(
                 kTraceError,
                 kTraceVideo,
                 -1,
                 "DirectDraw failed to EnableTransparentBackground HW don't support transparency");
    return -1;
    
}

int VideoRenderDirectDraw::FillSurface(DirectDrawSurface *pDDSurface,
                                           RECT* rect)
{
    
    if (NULL == pDDSurface)
    {
        return -1;
        
    }
    if (NULL == rect)
    {
        return -1;
        
    }

    
    HRESULT ddrval;
    DDBLTFX ddFX;

    ZeroMemory(&ddFX, sizeof(ddFX));
    ddFX.dwSize = sizeof(ddFX);
    ddFX.dwFillColor = RGB(0, 0, 0);

    
    ddrval = pDDSurface->Blt(rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT,
                             &ddFX);
    if (FAILED(ddrval))
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw failed to fill surface");
        return -1;
        
    }
    return 0;
}


bool VideoRenderDirectDraw::RemoteRenderingThreadProc(void *obj)
{
    return static_cast<VideoRenderDirectDraw*> (obj)->RemoteRenderingProcess();
}

bool VideoRenderDirectDraw::RemoteRenderingProcess()
{
    bool hwndChanged = false;
    int waitTime = 0;

    _screenEvent->Wait(100);

    _confCritSect->Enter();

    if (_blit == false)
    {
        _confCritSect->Leave();
        return true;
    }

    if (!::GetForegroundWindow())
    {
        
        _confCritSect->Leave();
        return true;
    }

    
    _processCount++;
    if (_deliverInQuarterFrameRate)
    {
        if (_processCount % 4 != 0)
        {
            _confCritSect->Leave();
            return true;
        }
    }
    else if (_deliverInHalfFrameRate)
    {
        if (_processCount % 2 != 0)
        {
            _confCritSect->Leave();
            return true;
        }
    }

    
    unsigned int startProcessTime = timeGetTime();

    hwndChanged = HasHWNDChanged();
    if (hwndChanged)
    {
        _clearMixingSurface = true;
    }

    std::map<int, DirectDrawChannel*>::iterator it;
    it = _directDrawChannels.begin();
    while (it != _directDrawChannels.end() && !_frameChanged)
    {
        if (it->second)
        {
            _frameChanged = it->second->IsOffScreenSurfaceUpdated(this);
        }
        it++;
    }
    if (_backSurface)
    {
        if (hwndChanged || _frameChanged)
        {
            BlitFromOffscreenBuffersToMixingBuffer();
            BlitFromBitmapBuffersToMixingBuffer();
            BlitFromTextToMixingBuffer();
        }
        BlitFromMixingBufferToBackBuffer();
        WaitAndFlip(waitTime);
    }
    else
    {
        if (hwndChanged || _frameChanged)
        {
            BlitFromOffscreenBuffersToMixingBuffer();
            BlitFromBitmapBuffersToMixingBuffer();
            BlitFromTextToMixingBuffer();
        }
        BlitFromMixingBufferToFrontBuffer(hwndChanged, waitTime);

    }
    
    
    const int totalRenderTime = ::timeGetTime() - startProcessTime - waitTime;
    DecideBestRenderingMode(hwndChanged, totalRenderTime);
    _frameChanged = false;
    _confCritSect->Leave();

    return true;
}
void VideoRenderDirectDraw::DecideBestRenderingMode(bool hwndChanged,
                                                        int totalRenderTime)
{
    








    const int timesSinceLastCPUCheck = timeGetTime()
            - _screenRenderCpuUsage.LastGetCpuTime();
    int cpu = 0;

    if (hwndChanged) 
    {
        cpu = _screenRenderCpuUsage.GetCpuUsage(); 
        _nrOfTooLongRenderTimes = 0; 
        return; 
    }
    
    if (_maxAllowedRenderTime > 0 && totalRenderTime > _maxAllowedRenderTime)
    {
        if (!_deliverInHalfFrameRate || totalRenderTime > 2
                * _maxAllowedRenderTime)
        {
            _nrOfTooLongRenderTimes += totalRenderTime / _maxAllowedRenderTime; 
        }
    }

    
    if (timesSinceLastCPUCheck > WindowsThreadCpuUsage::CPU_CHECK_INTERVAL)
    {
        cpu = _screenRenderCpuUsage.GetCpuUsage(); 
        WEBRTC_TRACE(
                     kTraceStream,
                     kTraceVideo,
                     -1,
                     "Screen render thread cpu usage. (Tid %d), cpu usage %d processTime %d, no of too long render times %d",
                     GetCurrentThreadId(), cpu, totalRenderTime,
                     _nrOfTooLongRenderTimes);

        
        
        if (cpu >= 5 && _renderModeWaitForCorrectScanLine == false
                && !_backSurface)
        {
            WEBRTC_TRACE(
                         kTraceWarning,
                         kTraceVideo,
                         -1,
                         "HIGH screen render thread cpu usage. (Tid %d), cpu usage %d, applying wait for scan line",
                         GetCurrentThreadId(), cpu);
            _renderModeWaitForCorrectScanLine = true;
            _fullScreenWaitEvent->StartTimer(true, 1);
        }
        else if (cpu >= 10 && _deliverInHalfFrameRate == false)
        {
            WEBRTC_TRACE(
                         kTraceWarning,
                         kTraceVideo,
                         -1,
                         "HIGH screen render thread cpu usage. (Tid %d), cpu usage %d, Render half rate",
                         GetCurrentThreadId(), cpu);
            _deliverInHalfFrameRate = true;
        }
        else
        {
            
            if (_nrOfTooLongRenderTimes > 15 || totalRenderTime
                    >= WindowsThreadCpuUsage::CPU_CHECK_INTERVAL)
            {

                
                if (_deliverInHalfFrameRate == false)
                {
                    WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                                 "Render half rate, tid: %d",
                                 GetCurrentThreadId());
                    _deliverInHalfFrameRate = true;
                }
                else if (_deliverInScreenType == false
                        && !_deliverInQuarterFrameRate)
                {
                    WEBRTC_TRACE(
                                 kTraceInfo,
                                 kTraceVideo,
                                 -1,
                                 "Applying deliver in screen type format, tid: %d",
                                 GetCurrentThreadId());
                    
                    std::map<int, DirectDrawChannel*>::iterator it;
                    it = _directDrawChannels.begin();
                    while (it != _directDrawChannels.end())
                    {
                        it->second->ChangeDeliverColorFormat(true);
                        it++;
                    }
                    _deliverInScreenType = true;
                }
                else if (_deliverInQuarterFrameRate == false)
                {
                    WEBRTC_TRACE(
                                 kTraceInfo,
                                 kTraceVideo,
                                 -1,
                                 "Render quarter rate and disable deliver in screen type format, tid: %d",
                                 GetCurrentThreadId());
                    _deliverInQuarterFrameRate = true;
                    if (_deliverInScreenType)
                    {
                        
                        std::map<int, DirectDrawChannel*>::iterator it;
                        it = _directDrawChannels.begin();
                        while (it != _directDrawChannels.end())
                        {
                            it->second->ChangeDeliverColorFormat(false);
                            it++;
                        }
                        _deliverInScreenType = false;
                    }
                }
                else if (_deliverInQuarterFrameRate == true
                        && !_deliverInScreenType)
                {
                    WEBRTC_TRACE(
                                 kTraceInfo,
                                 kTraceVideo,
                                 -1,
                                 "Render quarter rate and enable RGB fix, tid: %d",
                                 GetCurrentThreadId());
                    _deliverInQuarterFrameRate = true;

                    
                    std::map<int, DirectDrawChannel*>::iterator it;
                    it = _directDrawChannels.begin();
                    while (it != _directDrawChannels.end())
                    {
                        it->second->ChangeDeliverColorFormat(true);
                        it++;
                    }
                    _deliverInScreenType = true;
                }
            }
        }
        _nrOfTooLongRenderTimes = 0; 
    }
}





bool VideoRenderDirectDraw::HasHWNDChanged()
{
    
    if (!_fullscreen)
    {
        RECT currentRect;
        ::GetClientRect(_hWnd, &currentRect);
        if (!EqualRect(&currentRect, &_hwndRect))
        {
            int retVal = CreateMixingSurface(); 
            if (retVal != 0)
            {
                return false;
            }
            return true;
        }
    }
    return false;
}

int VideoRenderDirectDraw::BlitFromOffscreenBuffersToMixingBuffer()
{
    bool updateAll = false; 

    DDBLTFX ddbltfx;
    ZeroMemory(&ddbltfx, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwDDFX = DDBLTFX_NOTEARING;

    if (_mixingSurface == NULL)
    {
        int retVal = CreateMixingSurface();
        if (retVal != 0)
        {
            
            return retVal;
        }
    }
    RECT mixingRect;
    ::SetRectEmpty(&mixingRect);

    if (_fullscreen)
    {
        ::CopyRect(&mixingRect, &_screenRect);
    }
    else
    {
        ::CopyRect(&mixingRect, &_hwndRect);
        
        if (mixingRect.right > _screenRect.right)
        {
            mixingRect.right = _screenRect.right;
        }
        if (mixingRect.bottom > _screenRect.bottom)
        {
            mixingRect.bottom = _screenRect.bottom;
        }
    }
    if (!EqualRect(&_mixingRect, &mixingRect))
    {
        
        CopyRect(&_mixingRect, &mixingRect);
        FillSurface(_mixingSurface, &mixingRect);
        updateAll = true;
    }

    if (_clearMixingSurface)
    {
        FillSurface(_mixingSurface, &_mixingRect);
        _clearMixingSurface = false;
        updateAll = true;
    }

    std::multimap<int, unsigned int>::reverse_iterator it;
    it = _directDrawZorder.rbegin();
    while (it != _directDrawZorder.rend())
    {
        
        short streamID = (it->second >> 16);
        int channel = it->second & 0x0000ffff;

        std::map<int, DirectDrawChannel*>::iterator ddIt;
        ddIt = _directDrawChannels.find(channel);
        if (ddIt != _directDrawChannels.end())
        {
            
            DirectDrawChannel* channelObj = ddIt->second;
            if (channelObj && _mixingSurface)
            {
                if (updateAll || channelObj->IsOffScreenSurfaceUpdated(this))
                {
                    updateAll = true;
                    if (channelObj->BlitFromOffscreenBufferToMixingBuffer(
                                                                          this,
                                                                          streamID,
                                                                          _mixingSurface,
                                                                          _mixingRect,
                                                                          _demuxing)
                            != 0)
                    {
                        WEBRTC_TRACE(kTraceError, kTraceVideo,
                                     -1,
                                     "DirectDraw error BlitFromOffscreenBufferToMixingBuffer ");
                        _mixingSurface->Release();
                        _mixingSurface = NULL;
                    }
                }
            }
        }
        it++;
    }
    return 0;
}

int VideoRenderDirectDraw::BlitFromTextToMixingBuffer()
{
    if (_directDraw == NULL)
    {
        return -1;
    }
    if (!_mixingSurface)
    {
        return -1;
    }
    if (_textSettings.empty())
    {
        return 0;
    }

    HDC hdcDDSurface;
    HRESULT res = _mixingSurface->GetDC(&hdcDDSurface);
    if (res != S_OK)
    {
        return -1;
    }
    
    std::map<unsigned char, DirectDrawTextSettings*>::reverse_iterator it;
    it = _textSettings.rbegin();

    while (it != _textSettings.rend())
    {
        DirectDrawTextSettings* settings = it->second;
        it++;
        if (settings == NULL)
        {
            continue;
        }
        SetTextColor(hdcDDSurface, settings->_colorRefText);
        SetBkColor(hdcDDSurface, settings->_colorRefBackground);

        if (settings->_transparent)
        {
            SetBkMode(hdcDDSurface, TRANSPARENT); 
        }
        else
        {
            SetBkMode(hdcDDSurface, OPAQUE); 
        }
        RECT textRect;
        textRect.left = int(_mixingRect.right * settings->_textLeft);
        textRect.right = int(_mixingRect.right * settings->_textRight);
        textRect.top = int(_mixingRect.bottom * settings->_textTop);
        textRect.bottom = int(_mixingRect.bottom * settings->_textBottom);

        DrawTextA(hdcDDSurface, settings->_ptrText, settings->_textLength,
                  &textRect, DT_LEFT);
    }
    _mixingSurface->ReleaseDC(hdcDDSurface);
    return 0;
}

int VideoRenderDirectDraw::BlitFromBitmapBuffersToMixingBuffer()
{
    HRESULT ddrval;
    DDBLTFX ddbltfx;
    ZeroMemory(&ddbltfx, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwDDFX = DDBLTFX_NOTEARING;

    if (_directDraw == NULL)
    {
        return -1; 
    }

    std::map<unsigned char, DirectDrawBitmapSettings*>::reverse_iterator it;
    it = _bitmapSettings.rbegin();

    while (it != _bitmapSettings.rend())
    {
        DirectDrawBitmapSettings* settings = it->second;
        it++;
        if (settings == NULL)
        {
            continue;
        }

        
        
        if (_mixingSurface && settings->_transparentBitmapSurface
                && settings->_transparentBitmapWidth
                && settings->_transparentBitmapHeight)
        {
            DWORD signal = DDBLT_WAIT | DDBLT_DDFX;
            
            if (settings->_transparentBitmapColorKey)
            {
                signal |= DDBLT_KEYSRC;
                settings->_transparentBitmapSurface->SetColorKey(
                                                                 DDCKEY_SRCBLT,
                                                                 settings->_transparentBitmapColorKey);
            }

            
            RECT srcRect;
            SetRect(&srcRect, 0, 0, settings->_transparentBitmapWidth,
                    settings->_transparentBitmapHeight);

            RECT dstRect;
            if (settings->_transparentBitmapLeft
                    != settings->_transparentBitmapRight
                    && settings->_transparentBitmapTop
                            != settings->_transparentBitmapBottom)
            {
                CopyRect(&dstRect, &_mixingRect);
                dstRect.left = (int) (dstRect.right
                        * settings->_transparentBitmapLeft);
                dstRect.right = (int) (dstRect.right
                        * settings->_transparentBitmapRight);
                dstRect.top = (int) (dstRect.bottom
                        * settings->_transparentBitmapTop);
                dstRect.bottom = (int) (dstRect.bottom
                        * settings->_transparentBitmapBottom);
            }
            else
            {

                
                CopyRect(&dstRect, &srcRect);
                POINT startp;
                startp.x = (int) (_mixingRect.right
                        * settings->_transparentBitmapLeft);
                startp.y = (int) (_mixingRect.bottom
                        * settings->_transparentBitmapTop);
                OffsetRect(&dstRect, startp.x, startp.y);

                
                if (dstRect.bottom > _mixingRect.bottom)
                {
                    srcRect.bottom -= dstRect.bottom - _mixingRect.bottom;
                    
                    if (srcRect.bottom < 0)
                    {
                        srcRect.bottom = 0;
                    }
                    dstRect.bottom = _mixingRect.bottom;
                }
                if (dstRect.right > _mixingRect.right)
                {
                    srcRect.right -= dstRect.right - _mixingRect.right;
                    
                    if (srcRect.right < 0)
                    {
                        srcRect.right = 0;
                    }
                    dstRect.right = _mixingRect.right;
                }
            }
            

            
            ddrval = _mixingSurface->Blt(&dstRect,
                                         settings->_transparentBitmapSurface,
                                         &srcRect, signal, &ddbltfx);
            if (ddrval == DDERR_SURFACELOST)
            {
                if (!::GetForegroundWindow())
                {
                    
                    return 0;
                }
                
                settings->_transparentBitmapSurface->Release();
                settings->_transparentBitmapSurface = NULL;

                _clearMixingSurface = true;

                if (settings->_transparentBitMap)
                {
                    WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                                 "DirectDraw re-set transparent bitmap");
                    settings->SetBitmap(_trace, _directDraw);
                }
            }
            else if (ddrval != DD_OK)
            {
                settings->_transparentBitmapSurface->Release();
                settings->_transparentBitmapSurface = NULL;
                WEBRTC_TRACE(
                             kTraceInfo,
                             kTraceVideo,
                             -1,
                             "DirectDraw blt error 0x%x _transparentBitmapSurface",
                             ddrval);
                return -1;
                
            }
        }
    }
    return 0;
}




int VideoRenderDirectDraw::BlitFromMixingBufferToFrontBuffer(
                                                                 bool hwndChanged,
                                                                 int& waitTime)
{
    DDBLTFX ddbltfx;
    ZeroMemory(&ddbltfx, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwDDFX = DDBLTFX_NOTEARING;
    RECT rcRectDest;

    
    





    if (IsRectEmpty(&_mixingRect))
    {
        
        return 0;
    }
    if (_mixingSurface == NULL)
    {
        
        
        return 0;
    }
    if (_primarySurface == NULL)
    {
        int retVal = CreatePrimarySurface();
        if (retVal != 0)
        {
            
            return retVal;
        }
    }

    
    ::GetWindowRect(_hWnd, &rcRectDest);

    DWORD signal = DDBLT_WAIT | DDBLT_DDFX;

    
    if (_transparentBackground)
    {
        signal |= DDBLT_KEYSRC;
        DDCOLORKEY ColorKey;
        ColorKey.dwColorSpaceLowValue = RGB(0, 0, 0);
        ColorKey.dwColorSpaceHighValue = RGB(0, 0, 0);
        _mixingSurface->SetColorKey(DDCKEY_SRCBLT, &ColorKey);
    }

    if (_renderModeWaitForCorrectScanLine)
    {
        
        DWORD scanLines = 0;
        DWORD screenLines = _screenRect.bottom - 1; 
        DWORD screenLines90 = (screenLines * 9) / 10; 
        
        waitTime = ::timeGetTime();
        HRESULT hr = _directDraw->GetScanLine(&scanLines);
        while (screenLines90 > scanLines && hr == DD_OK)
        {
            _confCritSect->Leave();
            _fullScreenWaitEvent->Wait(3);
            _confCritSect->Enter();
            if (_directDraw == NULL)
            {
                return -1;
                
            }
            hr = _directDraw->GetScanLine(&scanLines);
        }
        
        waitTime = ::timeGetTime() - waitTime;
    }

    HRESULT ddrval = _primarySurface->Blt(&rcRectDest, _mixingSurface,
                                          &_mixingRect, signal, &ddbltfx);
    if (ddrval == DDERR_SURFACELOST)
    {
        if (!::GetForegroundWindow())
        {
            
            return 0;
        }
        ddrval = _primarySurface->Restore();
        if (ddrval == DD_OK) 
        {
            ddrval = _primarySurface->Blt(&rcRectDest, _mixingSurface,
                                          &_mixingRect, signal, &ddbltfx);
        }
        if (ddrval != DD_OK) 
        {
            WEBRTC_TRACE(
                         kTraceWarning,
                         kTraceVideo,
                         -1,
                         "DirectDraw failed to restore lost _primarySurface  0x%x",
                         ddrval);
            _primarySurface->Release();
            _primarySurface = NULL;
            if (_mixingSurface)
            {
                _mixingSurface->Release();
                _mixingSurface = NULL;
            }
            return -1;
            
        }
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw restored lost _primarySurface");
    }
    else if (ddrval == DDERR_EXCEPTION)
    {
        _primarySurface->Release();
        _primarySurface = NULL;
        if (_mixingSurface)
        {
            _mixingSurface->Release();
            _mixingSurface = NULL;
        }
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw exception in _primarySurface");
        return -1;
        
    }
    if (ddrval != DD_OK)
    {
        if (ddrval != 0x80004005) 
        {
            WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                         "DirectDraw blt error 0x%x _primarySurface", ddrval);
            return -1;
            
        }
    }
    return 0;
}





int VideoRenderDirectDraw::WaitAndFlip(int& waitTime)
{
    if (_primarySurface == NULL)
    {
        
        return -1;
        
    }
    if (_directDraw == NULL)
    {
        return -1;
        
    }
    
    DWORD scanLines = 0;
    DWORD screenLines = _screenRect.bottom - 1; 
    DWORD screenLines90 = (screenLines * 9) / 10; 

    
    waitTime = ::timeGetTime();
    HRESULT hr = _directDraw->GetScanLine(&scanLines);
    while (screenLines90 > scanLines && hr == DD_OK)
    {
        _confCritSect->Leave();
        _fullScreenWaitEvent->Wait(3);
        _confCritSect->Enter();
        if (_directDraw == NULL)
        {
            return -1;
            
        }
        hr = _directDraw->GetScanLine(&scanLines);
    }
    
    waitTime = ::timeGetTime() - waitTime;
    if (screenLines > scanLines)
    {
        
        _directDraw->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, NULL);
    }

    
    HRESULT ddrval = _primarySurface->Flip(NULL, DDFLIP_WAIT); 
    if (ddrval == DDERR_SURFACELOST)
    {
        if (!::GetForegroundWindow())
        {
            
            return 0;
        }
        
        
        
        
        
        
        
        
        
        
        
        ddrval = _primarySurface->Restore();
        if (ddrval != DD_OK)
        {
            WEBRTC_TRACE(
                         kTraceWarning,
                         kTraceVideo,
                         -1,
                         "DirectDraw failed to restore _primarySurface, in flip, 0x%x",
                         ddrval);
            return -1;
            
        }
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw restore _primarySurface in flip");

    }
    else if (ddrval != DD_OK)
    {
        return -1;
        
    }
    return 0;
}

int VideoRenderDirectDraw::BlitFromMixingBufferToBackBuffer()
{
    if (_backSurface == NULL)
    {
        return -1;
        
    }
    if (IsRectEmpty(&_mixingRect))
    {
        
        return 0;
    }
    DDBLTFX ddbltfx;
    ZeroMemory(&ddbltfx, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwDDFX = DDBLTFX_NOTEARING;

    
    HRESULT ddrval = _backSurface->Blt(&_screenRect, _mixingSurface,
                                       &_mixingRect, DDBLT_WAIT | DDBLT_DDFX,
                                       &ddbltfx);
    if (ddrval == DDERR_SURFACELOST)
    {
        if (!::GetForegroundWindow())
        {
            
            return 0;
        }
        
        
        
        
        
        
        
        
        
        
        
        
        ddrval = _primarySurface->Restore();
        if (ddrval != DD_OK)
        {
            WEBRTC_TRACE(kTraceWarning, kTraceVideo, -1,
                         "DirectDraw failed to restore _primarySurface");
            return -1;
            
        }
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                     "DirectDraw restored _primarySurface");

        _clearMixingSurface = true;

    }
    else if (ddrval != DD_OK)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "DirectDraw blt error 0x%x _backSurface", ddrval);
        return -1;
        
    }
    return 0;
}



































































































































































WebRtc_Word32 VideoRenderDirectDraw::StartRender()
{
    WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
    return 0;
}

WebRtc_Word32 VideoRenderDirectDraw::StopRender()
{
    WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
    return 0;
}

WebRtc_Word32 VideoRenderDirectDraw::ChangeWindow(void* window)
{
    WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
    return -1;
}

} 

