









#include "testAPI.h"

#include <stdio.h>

#if defined(_WIN32)
#include <tchar.h>
#include <windows.h>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <windows.h>
#include <ddraw.h>

#elif defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)

#include <iostream>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/time.h>

#endif

#include "common_types.h"
#include "process_thread.h"
#include "module_common_types.h"
#include "video_render_defines.h"
#include "video_render.h"
#include "tick_util.h"
#include "trace.h"

using namespace webrtc;

void GetTestVideoFrame(WebRtc_UWord8* frame,
                       WebRtc_Word32 width,
                       WebRtc_Word32 height,
                       WebRtc_UWord8 startColor);
int TestSingleStream(VideoRender* renderModule);
int TestFullscreenStream(VideoRender* &renderModule,
                         void* window,
                         const VideoRenderType videoRenderType);
int TestBitmapText(VideoRender* renderModule);
int TestMultipleStreams(VideoRender* renderModule);
int TestExternalRender(VideoRender* renderModule);

#define TEST_FRAME_RATE 30
#define TEST_TIME_SECOND 5
#define TEST_FRAME_NUM (TEST_FRAME_RATE*TEST_TIME_SECOND)
#define TEST_STREAM0_START_COLOR 0
#define TEST_STREAM1_START_COLOR 64
#define TEST_STREAM2_START_COLOR 128
#define TEST_STREAM3_START_COLOR 192

#if defined(_WIN32) && defined(_DEBUG)

#define SLEEP(x) ::Sleep(x)
#elif defined(WEBRTC_LINUX)

#define GET_TIME_IN_MS timeGetTime()
#define SLEEP(x) Sleep(x)

void Sleep(unsigned long x)
{
    timespec t;
    t.tv_sec = x/1000;
    t.tv_nsec = (x-(x/1000)*1000)*1000000;
    nanosleep(&t,NULL);
}

unsigned long timeGetTime()
{
    struct timeval tv;
    struct timezone tz;
    unsigned long val;

    gettimeofday(&tv, &tz);
    val= tv.tv_sec*1000+ tv.tv_usec/1000;
    return(val);
}

#elif defined(WEBRTC_MAC_INTEL)

#include <unistd.h>

#define GET_TIME_IN_MS timeGetTime()
#define SLEEP(x) usleep(x * 1000)

unsigned long timeGetTime()
{
    return 0;
}

#else

#define GET_TIME_IN_MS ::timeGetTime()
#define SLEEP(x) ::Sleep(x)

#endif

using namespace std;

#if defined(_WIN32)
LRESULT CALLBACK WebRtcWinProc( HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_DESTROY:
        break;
        case WM_COMMAND:
        break;
    }
    return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

int WebRtcCreateWindow(HWND &hwndMain,int winNum, int width, int height)
{
    HINSTANCE hinst = GetModuleHandle(0);
    WNDCLASSEX wcx;
    wcx.hInstance = hinst;
    wcx.lpszClassName = TEXT("VideoRenderTest");
    wcx.lpfnWndProc = (WNDPROC)WebRtcWinProc;
    wcx.style = CS_DBLCLKS;
    wcx.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wcx.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wcx.hCursor = LoadCursor (NULL, IDC_ARROW);
    wcx.lpszMenuName = NULL;
    wcx.cbSize = sizeof (WNDCLASSEX);
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hbrBackground = GetSysColorBrush(COLOR_3DFACE);

    
    
    if ( !RegisterClassEx (&wcx) )
    {
        MessageBox( 0, TEXT("Failed to register window class!"),TEXT("Error!"), MB_OK|MB_ICONERROR );
        return 0;
    }

    
    hwndMain = CreateWindowEx(
            0, 
            TEXT("VideoRenderTest"), 
            TEXT("VideoRenderTest Window"), 
            WS_OVERLAPPED |WS_THICKFRAME, 
            800, 
            0, 
            width, 
            height, 
            (HWND) NULL, 
            (HMENU) NULL, 
            hinst, 
            NULL); 

    if (!hwndMain)
        return -1;

    
    
    

    ShowWindow(hwndMain, SW_SHOWDEFAULT);
    UpdateWindow(hwndMain);
    return 0;
}

#elif defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)

int WebRtcCreateWindow(Window *outWindow, Display **outDisplay, int winNum, int width, int height) 

{
    int screen, xpos = 10, ypos = 10;
    XEvent evnt;
    XSetWindowAttributes xswa; 
    XVisualInfo vinfo; 
    unsigned long mask; 

    
    Display* _display = XOpenDisplay( NULL );

    
    screen = DefaultScreen(_display);

    
    if( XMatchVisualInfo(_display, screen, 24, TrueColor, &vinfo) != 0 )
    {
        
    }

    
    xswa.colormap = XCreateColormap(_display, DefaultRootWindow(_display), vinfo.visual, AllocNone);
    xswa.event_mask = StructureNotifyMask | ExposureMask;
    xswa.background_pixel = 0;
    xswa.border_pixel = 0;

    
    mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    switch( winNum )
    {
        case 0:
        xpos = 200;
        ypos = 200;
        break;
        case 1:
        xpos = 300;
        ypos = 200;
        break;
        default:
        break;
    }

    
    Window _window = XCreateWindow(_display, DefaultRootWindow(_display),
            xpos, ypos,
            width,
            height,
            0, vinfo.depth,
            InputOutput,
            vinfo.visual,
            mask, &xswa);

    
    if( winNum == 0 )
    {
        XStoreName(_display, _window, "VE MM Local Window");
        XSetIconName(_display, _window, "VE MM Local Window");
    }
    else if( winNum == 1 )
    {
        XStoreName(_display, _window, "VE MM Remote Window");
        XSetIconName(_display, _window, "VE MM Remote Window");
    }

    
    XSelectInput(_display, _window, StructureNotifyMask);

    
    XMapWindow(_display, _window);

    
    do
    {
        XNextEvent(_display, &evnt);
    }
    while (evnt.type != MapNotify || evnt.xmap.event != _window);

    *outWindow = _window;
    *outDisplay = _display;

    return 0;
}
#endif  



class MyRenderCallback: public VideoRenderCallback
{
public:
    MyRenderCallback() :
        _cnt(0)
    {
    }
    ;
    ~MyRenderCallback()
    {
    }
    ;
    virtual WebRtc_Word32 RenderFrame(const WebRtc_UWord32 streamId,
                                      VideoFrame& videoFrame)
    {
        _cnt++;
        if (_cnt % 100 == 0)
        {
            printf("Render callback %d \n",_cnt);
        }
        return 0;
    }
    WebRtc_Word32 _cnt;
};

void GetTestVideoFrame(WebRtc_UWord8* frame,
                       WebRtc_Word32 width,
                       WebRtc_Word32 height,
                       WebRtc_UWord8 startColor) {
    
    static WebRtc_UWord8 color = startColor;

    WebRtc_UWord8* destY = frame;
    WebRtc_UWord8* destU = &frame[width*height];
    WebRtc_UWord8* destV = &frame[width*height*5/4];
    
    for (WebRtc_Word32 y=0; y<(width*height); y++)
    {
      destY[y] = color;
    }
    
    for (WebRtc_Word32 u=0; u<(width*height/4); u++)
    {
      destU[u] = color;
    }
    
    for (WebRtc_Word32 v=0; v<(width*height/4); v++)
    {
      destV[v] = color;
    }

    color++;
}

int TestSingleStream(VideoRender* renderModule) {
    int error = 0;
    
    printf("Add stream 0 to entire window\n");
    const int streamId0 = 0;
    VideoRenderCallback* renderCallback0 = renderModule->AddIncomingRenderStream(streamId0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
    assert(renderCallback0 != NULL);

#ifndef WEBRTC_INCLUDE_INTERNAL_VIDEO_RENDER
    MyRenderCallback externalRender;
    renderModule->AddExternalRenderCallback(streamId0, &externalRender);
#endif

    printf("Start render\n");
    error = renderModule->StartRender(streamId0);
    if (error != 0) {
      
      
      
      assert(false);
    }

    
    const WebRtc_UWord32 width = 352;
    const WebRtc_UWord32 height = 288;
    const WebRtc_UWord32 numBytes = (WebRtc_UWord32)(1.5 * width * height);

    VideoFrame videoFrame0;
    videoFrame0.VerifyAndAllocate(numBytes);

    const WebRtc_UWord32 renderDelayMs = 500;

    for (int i=0; i<TEST_FRAME_NUM; i++) {
        GetTestVideoFrame(videoFrame0.Buffer(), width, height, TEST_STREAM0_START_COLOR);
        videoFrame0.SetRenderTime(TickTime::MillisecondTimestamp() + renderDelayMs); 
        videoFrame0.SetWidth(width);
        videoFrame0.SetHeight(height);
        videoFrame0.SetLength(numBytes);
        renderCallback0->RenderFrame(streamId0, videoFrame0);
        SLEEP(1000/TEST_FRAME_RATE);
    }

    videoFrame0.Free();

    
    printf("Closing...\n");
    error = renderModule->StopRender(streamId0);
    assert(error == 0);

    error = renderModule->DeleteIncomingRenderStream(streamId0);
    assert(error == 0);

    return 0;
}

int TestFullscreenStream(VideoRender* &renderModule,
                         void* window,
                         const VideoRenderType videoRenderType) {
    VideoRender::DestroyVideoRender(renderModule);
    renderModule = VideoRender::CreateVideoRender(12345, window, true, videoRenderType);

    TestSingleStream(renderModule);

    VideoRender::DestroyVideoRender(renderModule);
    renderModule = VideoRender::CreateVideoRender(12345, window, false, videoRenderType);

    return 0;
}

int TestBitmapText(VideoRender* renderModule) {
#if defined(WIN32)

    int error = 0;
    
    printf("Add stream 0 to entire window\n");
    const int streamId0 = 0;
    VideoRenderCallback* renderCallback0 = renderModule->AddIncomingRenderStream(streamId0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
    assert(renderCallback0 != NULL);

    printf("Adding Bitmap\n");
    DDCOLORKEY ColorKey; 
    ColorKey.dwColorSpaceHighValue = RGB(0, 0, 0);
    ColorKey.dwColorSpaceLowValue = RGB(0, 0, 0);
    HBITMAP hbm = (HBITMAP)LoadImage(NULL,
                                     (LPCTSTR)_T("renderStartImage.bmp"),
                                     IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    renderModule->SetBitmap(hbm, 0, &ColorKey, 0.0f, 0.0f, 0.3f,
                             0.3f);

    printf("Adding Text\n");
    renderModule->SetText(1, (WebRtc_UWord8*) "WebRtc Render Demo App", 20,
                           RGB(255, 0, 0), RGB(0, 0, 0), 0.25f, 0.1f, 1.0f,
                           1.0f);

    printf("Start render\n");
    error = renderModule->StartRender(streamId0);
    assert(error == 0);

        
    const WebRtc_UWord32 width = 352;
    const WebRtc_UWord32 height = 288;
    const WebRtc_UWord32 numBytes = (WebRtc_UWord32)(1.5 * width * height);

    VideoFrame videoFrame0;
    videoFrame0.VerifyAndAllocate(numBytes);

    const WebRtc_UWord32 renderDelayMs = 500;

    for (int i=0; i<TEST_FRAME_NUM; i++) {
        GetTestVideoFrame(videoFrame0.Buffer(), width, height, TEST_STREAM0_START_COLOR);
        videoFrame0.SetRenderTime(TickTime::MillisecondTimestamp() + renderDelayMs); 
        videoFrame0.SetWidth(width);
        videoFrame0.SetHeight(height);
        videoFrame0.SetLength(numBytes);
        renderCallback0->RenderFrame(streamId0, videoFrame0);
        SLEEP(1000/TEST_FRAME_RATE);
    }
    videoFrame0.Free();
    
    SLEEP(renderDelayMs*2);


    
    printf("Closing...\n");
    ColorKey.dwColorSpaceHighValue = RGB(0,0,0);
    ColorKey.dwColorSpaceLowValue = RGB(0,0,0);
    renderModule->SetBitmap(NULL, 0, &ColorKey, 0.0f, 0.0f, 0.0f, 0.0f);
    renderModule->SetText(1, NULL, 20, RGB(255,255,255),
                    RGB(0,0,0), 0.0f, 0.0f, 0.0f, 0.0f);

    error = renderModule->StopRender(streamId0);
    assert(error == 0);

    error = renderModule->DeleteIncomingRenderStream(streamId0);
    assert(error == 0);
#endif

    return 0;
}

int TestMultipleStreams(VideoRender* renderModule) {
    
    printf("Add stream 0\n");
    const int streamId0 = 0;
    VideoRenderCallback* renderCallback0 =
        renderModule->AddIncomingRenderStream(streamId0, 0, 0.0f, 0.0f, 0.45f, 0.45f);
    assert(renderCallback0 != NULL);
    printf("Add stream 1\n");
    const int streamId1 = 1;
    VideoRenderCallback* renderCallback1 =
        renderModule->AddIncomingRenderStream(streamId1, 0, 0.55f, 0.0f, 1.0f, 0.45f);
    assert(renderCallback1 != NULL);
    printf("Add stream 2\n");
    const int streamId2 = 2;
    VideoRenderCallback* renderCallback2 =
        renderModule->AddIncomingRenderStream(streamId2, 0, 0.0f, 0.55f, 0.45f, 1.0f);
    assert(renderCallback2 != NULL);
    printf("Add stream 3\n");
    const int streamId3 = 3;
    VideoRenderCallback* renderCallback3 =
        renderModule->AddIncomingRenderStream(streamId3, 0, 0.55f, 0.55f, 1.0f, 1.0f);
    assert(renderCallback3 != NULL);
    assert(renderModule->StartRender(streamId0) == 0);
    assert(renderModule->StartRender(streamId1) == 0);
    assert(renderModule->StartRender(streamId2) == 0);
    assert(renderModule->StartRender(streamId3) == 0);

    
    const WebRtc_UWord32 width = 352;
    const WebRtc_UWord32 height = 288;
    const WebRtc_UWord32 numBytes = (WebRtc_UWord32)(1.5 * width * height);

    VideoFrame videoFrame0;
    videoFrame0.VerifyAndAllocate(numBytes);
    VideoFrame videoFrame1;
    videoFrame1.VerifyAndAllocate(numBytes);
    VideoFrame videoFrame2;
    videoFrame2.VerifyAndAllocate(numBytes);
    VideoFrame videoFrame3;
    videoFrame3.VerifyAndAllocate(numBytes);

    const WebRtc_UWord32 renderDelayMs = 500;

    for (int i=0; i<TEST_FRAME_NUM; i++) {
        GetTestVideoFrame(videoFrame0.Buffer(), width, height, TEST_STREAM0_START_COLOR);
        videoFrame0.SetRenderTime(TickTime::MillisecondTimestamp() + renderDelayMs); 
        videoFrame0.SetWidth(width);
        videoFrame0.SetHeight(height);
        videoFrame0.SetLength(numBytes);
        renderCallback0->RenderFrame(streamId0, videoFrame0);

        GetTestVideoFrame(videoFrame1.Buffer(), width, height, TEST_STREAM1_START_COLOR);
        videoFrame1.SetRenderTime(TickTime::MillisecondTimestamp() + renderDelayMs); 
        videoFrame1.SetWidth(width);
        videoFrame1.SetHeight(height);
        videoFrame1.SetLength(numBytes);
        renderCallback1->RenderFrame(streamId1, videoFrame1);

        GetTestVideoFrame(videoFrame2.Buffer(), width, height, TEST_STREAM2_START_COLOR);
        videoFrame2.SetRenderTime(TickTime::MillisecondTimestamp() + renderDelayMs); 
        videoFrame2.SetWidth(width);
        videoFrame2.SetHeight(height);
        videoFrame2.SetLength(numBytes);
        renderCallback2->RenderFrame(streamId2, videoFrame2);

        GetTestVideoFrame(videoFrame3.Buffer(), width, height, TEST_STREAM3_START_COLOR);
        videoFrame3.SetRenderTime(TickTime::MillisecondTimestamp() + renderDelayMs); 
        videoFrame3.SetWidth(width);
        videoFrame3.SetHeight(height);
        videoFrame3.SetLength(numBytes);
        renderCallback3->RenderFrame(streamId3, videoFrame3);

        SLEEP(1000/TEST_FRAME_RATE);
    }

    videoFrame0.Free();
    videoFrame1.Free();
    videoFrame2.Free();
    videoFrame3.Free();

    
    printf("Closing...\n");
    assert(renderModule->StopRender(streamId0) == 0);
    assert(renderModule->DeleteIncomingRenderStream(streamId0) == 0);
    assert(renderModule->StopRender(streamId1) == 0);
    assert(renderModule->DeleteIncomingRenderStream(streamId1) == 0);
    assert(renderModule->StopRender(streamId2) == 0);
    assert(renderModule->DeleteIncomingRenderStream(streamId2) == 0);
    assert(renderModule->StopRender(streamId3) == 0);
    assert(renderModule->DeleteIncomingRenderStream(streamId3) == 0);

    return 0;
}

int TestExternalRender(VideoRender* renderModule) {
    MyRenderCallback *externalRender = new MyRenderCallback();

    const int streamId0 = 0;
    VideoRenderCallback* renderCallback0 =
        renderModule->AddIncomingRenderStream(streamId0, 0, 0.0f, 0.0f,
                                                   1.0f, 1.0f);
    assert(renderCallback0 != NULL);
    assert(renderModule->AddExternalRenderCallback(streamId0,
                                                   externalRender) == 0);

    assert(renderModule->StartRender(streamId0) == 0);

    const WebRtc_UWord32 width = 352;
    const WebRtc_UWord32 height = 288;
    const WebRtc_UWord32 numBytes = (WebRtc_UWord32) (1.5 * width * height);
    VideoFrame videoFrame0;
    videoFrame0.VerifyAndAllocate(numBytes);

    const WebRtc_UWord32 renderDelayMs = 500;
    int frameCount = TEST_FRAME_NUM;
    for (int i=0; i<frameCount; i++) {
        videoFrame0.SetRenderTime(TickTime::MillisecondTimestamp() + renderDelayMs);
        videoFrame0.SetWidth(width);
        videoFrame0.SetHeight(height);
        renderCallback0->RenderFrame(streamId0, videoFrame0);
        SLEEP(33);
    }

    
    SLEEP(2*renderDelayMs);
    videoFrame0.Free();

    assert(renderModule->StopRender(streamId0) == 0);
    assert(renderModule->DeleteIncomingRenderStream(streamId0) == 0);
    assert(frameCount == externalRender->_cnt);

    delete externalRender;
    externalRender = NULL;

    return 0;
}

void RunVideoRenderTests(void* window, VideoRenderType windowType) {
#ifndef WEBRTC_INCLUDE_INTERNAL_VIDEO_RENDER
    windowType = kRenderExternal;
#endif

    int myId = 12345;

    
    printf("Create render module\n");
    VideoRender* renderModule = NULL;
    renderModule = VideoRender::CreateVideoRender(myId,
                                                  window,
                                                  false,
                                                  windowType);
    assert(renderModule != NULL);


    
    printf("#### TestSingleStream ####\n");
    if (TestSingleStream(renderModule) != 0) {
        printf ("TestSingleStream failed\n");
    }

    
    printf("#### TestFullscreenStream ####\n");
    if (TestFullscreenStream(renderModule, window, windowType) != 0) {
        printf ("TestFullscreenStream failed\n");
    }

    
    printf("#### TestBitmapText ####\n");
    if (TestBitmapText(renderModule) != 0) {
        printf ("TestBitmapText failed\n");
    }

    
    printf("#### TestMultipleStreams ####\n");
    if (TestMultipleStreams(renderModule) != 0) {
        printf ("TestMultipleStreams failed\n");
    }

    
    printf("#### TestExternalRender ####\n");
    if (TestExternalRender(renderModule) != 0) {
        printf ("TestExternalRender failed\n");
    }

    delete renderModule;
    renderModule = NULL;

    printf("VideoRender unit tests passed.\n");
}


#if defined(_WIN32)
int _tmain(int argc, _TCHAR* argv[])
#elif defined(WEBRTC_LINUX) && !defined(WEBRTC_ANDROID)
int main(int argc, char* argv[])
#endif
#if !defined(WEBRTC_MAC) && !defined(WEBRTC_ANDROID)
{
    
    void* window = NULL;
#if defined (_WIN32)
    HWND testHwnd;
    WebRtcCreateWindow(testHwnd, 0, 352, 288);
    window = (void*)testHwnd;
    VideoRenderType windowType = kRenderWindows;
#elif defined(WEBRTC_LINUX)
    Window testWindow;
    Display* display;
    WebRtcCreateWindow(&testWindow, &display, 0, 352, 288);
    VideoRenderType windowType = kRenderX11;
    window = (void*)testWindow;
#endif 

    RunVideoRenderTests(window, windowType);
    return 0;
}
#endif  
