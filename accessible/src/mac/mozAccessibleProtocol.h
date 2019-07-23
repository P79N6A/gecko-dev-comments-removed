





































#import <Cocoa/Cocoa.h>

#import "mozView.h"











@protocol mozAccessible



- (BOOL)isRoot;






- (BOOL)hasRepresentedView;
- (id)representedView;

#ifdef DEBUG


- (void)printHierarchy;
#endif




- (id)accessibilityHitTest:(NSPoint)point;


- (BOOL)accessibilityIsIgnored;


- (id)accessibilityFocusedUIElement;




- (NSArray*)accessibilityAttributeNames;


- (id)accessibilityAttributeValue:(NSString*)attribute;


- (BOOL)accessibilityIsAttributeSettable:(NSString*)attribute;



- (NSArray*)accessibilityActionNames;
- (NSString*)accessibilityActionDescription:(NSString*)action;
- (void)accessibilityPerformAction:(NSString*)action;

@end

