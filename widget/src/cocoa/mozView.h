




































#ifndef mozView_h_
#define mozView_h_

#undef DARWIN
#import <Cocoa/Cocoa.h>
class nsIWidget;





@protocol mozView

  
- (nsIWidget*)widget;

  
- (NSMenu*)contextMenu;

  
  
- (void)setNeedsPendingDisplay;
- (void)setNeedsPendingDisplayInRect:(NSRect)invalidRect;

  
- (void)widgetDestroyed;

- (BOOL)isDragInProgress;

@end






@interface NSObject(mozWindow)

- (BOOL)suppressMakeKeyFront;
- (void)setSuppressMakeKeyFront:(BOOL)inSuppress;

@end

#endif 
