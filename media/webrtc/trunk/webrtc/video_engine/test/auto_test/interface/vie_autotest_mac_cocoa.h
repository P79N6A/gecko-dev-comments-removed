









#include "engine_configurations.h"

#if defined(COCOA_RENDERING)

#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_MAC_COCOA_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_MAC_COCOA_H_

#include "vie_autotest_window_manager_interface.h"

@class CocoaRenderView;

#import <Cocoa/Cocoa.h>

@interface TestCocoaUi : NSObject {
  CocoaRenderView* cocoaRenderView1_;
  CocoaRenderView* cocoaRenderView2_;
  NSWindow* window1_;
  NSWindow* window2_;

  AutoTestRect window1Size_;
  AutoTestRect window2Size_;
  void* window1Title_;
  void* window2Title_;
}


- (void)createWindows:(NSObject*)ignored;


- (void)prepareToCreateWindowsWithSize:(AutoTestRect)window1Size
                               andSize:(AutoTestRect)window2Size
                             withTitle:(void*)window1Title
                              andTitle:(void*)window2Title;

- (NSWindow*)window1;
- (NSWindow*)window2;
- (CocoaRenderView*)cocoaRenderView1;
- (CocoaRenderView*)cocoaRenderView2;

@end

class ViEAutoTestWindowManager: public ViEAutoTestWindowManagerInterface {
 public:
  ViEAutoTestWindowManager();
  virtual ~ViEAutoTestWindowManager();
  virtual void* GetWindow1();
  virtual void* GetWindow2();
  virtual int CreateWindows(AutoTestRect window1Size,
                            AutoTestRect window2Size,
                            void* window1Title,
                            void* window2Title);
  virtual int TerminateWindows();
  virtual bool SetTopmostWindow();

 private:
  TestCocoaUi* cocoa_ui_;
};

#endif  
#endif  
