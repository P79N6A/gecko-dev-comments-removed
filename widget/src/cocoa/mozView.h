




































#ifndef mozView_h_
#define mozView_h_

#include "npapi.h"

#undef DARWIN
#import <Cocoa/Cocoa.h>
class nsIWidget;

namespace mozilla {
namespace widget{
class TextInputHandler;
} 
} 





@protocol mozView

  
  
  
- (void)installTextInputHandler:(mozilla::widget::TextInputHandler*)aHandler;
- (void)uninstallTextInputHandler;

  
- (nsIWidget*)widget;

  
- (NSMenu*)contextMenu;

  
  
- (void)setNeedsPendingDisplay;
- (void)setNeedsPendingDisplayInRect:(NSRect)invalidRect;

  
- (void)widgetDestroyed;

- (BOOL)isDragInProgress;

  
- (NPEventModel)pluginEventModel;

  
- (BOOL)isFirstResponder;

  
- (void)maybeInitContextMenuTracking;

  
- (BOOL)isPluginView;

@end






@interface NSObject(mozWindow)

- (BOOL)suppressMakeKeyFront;
- (void)setSuppressMakeKeyFront:(BOOL)inSuppress;

@end

#endif 
