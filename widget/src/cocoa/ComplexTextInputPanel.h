


























#ifndef ComplexTextInputPanel_h_
#define ComplexTextInputPanel_h_

#import <Cocoa/Cocoa.h>

@interface ComplexTextInputPanel : NSPanel {
  NSTextView *mInputTextView;
}

+ (ComplexTextInputPanel*)sharedComplexTextInputPanel;

- (NSTextInputContext*)inputContext;
- (BOOL)interpretKeyEvent:(NSEvent*)event string:(NSString**)string;
- (void)cancelComposition;

@end

#endif 
