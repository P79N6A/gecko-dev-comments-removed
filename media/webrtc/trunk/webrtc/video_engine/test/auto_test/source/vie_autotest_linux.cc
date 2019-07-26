












#include "webrtc/video_engine/test/auto_test/interface/vie_autotest_linux.h"

#include <string>

#include "webrtc/engine_configurations.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest_defines.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest_main.h"

ViEAutoTestWindowManager::ViEAutoTestWindowManager()
    : _hdsp1(NULL),
      _hdsp2(NULL) {
}

ViEAutoTestWindowManager::~ViEAutoTestWindowManager() {
  TerminateWindows();
}

void* ViEAutoTestWindowManager::GetWindow1() {
  return reinterpret_cast<void*>(_hwnd1);
}

void* ViEAutoTestWindowManager::GetWindow2() {
  return reinterpret_cast<void*>(_hwnd2);
}

int ViEAutoTestWindowManager::TerminateWindows() {
  if (_hdsp1) {
    ViEDestroyWindow(&_hwnd1, _hdsp1);
    _hdsp1 = NULL;
  }
  if (_hdsp2) {
    ViEDestroyWindow(&_hwnd2, _hdsp2);
    _hdsp2 = NULL;
  }
  return 0;
}

int ViEAutoTestWindowManager::CreateWindows(AutoTestRect window1Size,
                                            AutoTestRect window2Size,
                                            void* window1Title,
                                            void* window2Title) {
  ViECreateWindow(&_hwnd1, &_hdsp1, window1Size.origin.x,
                  window1Size.origin.y, window1Size.size.width,
                  window1Size.size.height,
                  reinterpret_cast<char*>(window1Title));
  ViECreateWindow(&_hwnd2, &_hdsp2, window2Size.origin.x,
                  window2Size.origin.y, window2Size.size.width,
                  window2Size.size.height,
                  reinterpret_cast<char*>(window2Title));

  return 0;
}

int ViEAutoTestWindowManager::ViECreateWindow(Window *out_window,
                                              Display **out_display, int x_pos,
                                              int y_pos, int width, int height,
                                              char* title) {
  Display* display = XOpenDisplay(NULL);
  if (display == NULL) {
    
    printf("Failed to connect to X server: X environment likely broken\n");
    exit(-1);
  }

  int screen = DefaultScreen(display);

  
  
  XVisualInfo visual_info;
  if (XMatchVisualInfo(display, screen, 24, TrueColor, &visual_info) == 0) {
    printf("Failed to establish 24-bit TrueColor in X environment.\n");
    exit(-1);
  }

  
  XSetWindowAttributes window_attributes;
  window_attributes.colormap = XCreateColormap(
      display, DefaultRootWindow(display), visual_info.visual, AllocNone);
  window_attributes.event_mask = StructureNotifyMask | ExposureMask;
  window_attributes.background_pixel = 0;
  window_attributes.border_pixel = 0;

  unsigned long attribute_mask = CWBackPixel | CWBorderPixel | CWColormap |
                                 CWEventMask;

  Window _window = XCreateWindow(display, DefaultRootWindow(display), x_pos,
                                 y_pos, width, height, 0, visual_info.depth,
                                 InputOutput, visual_info.visual,
                                 attribute_mask, &window_attributes);

  
  XStoreName(display, _window, title);
  XSetIconName(display, _window, title);

  
  XSelectInput(display, _window, StructureNotifyMask);

  
  XMapWindow(display, _window);

  
  XEvent event;
  do {
    XNextEvent(display, &event);
  } while (event.type != MapNotify || event.xmap.event != _window);

  *out_window = _window;
  *out_display = display;
  return 0;
}

int ViEAutoTestWindowManager::ViEDestroyWindow(Window *window,
                                               Display *display) {
  XUnmapWindow(display, *window);
  XDestroyWindow(display, *window);
  XSync(display, false);
  XCloseDisplay(display);
  return 0;
}

bool ViEAutoTestWindowManager::SetTopmostWindow() {
  return 0;
}

int main(int argc, char** argv) {
  ViEAutoTestMain auto_test;
  return auto_test.RunTests(argc, argv);
}
