





































#include "nsAccessibleWrap.h"

#import <Cocoa/Cocoa.h>

#import "mozAccessibleProtocol.h"

@class mozRootAccessible;

@interface mozAccessible : NSObject <mozAccessible>
{
  nsAccessibleWrap *mGeckoAccessible;  
  NSMutableArray   *mChildren;         
  
  
  
  
  
  BOOL mIsExpired;
  
  
  PRUint32        mRole;
}


- (id)initWithAccessible:(nsAccessibleWrap*)geckoParent;


- (id <mozAccessible>)parent;


- (NSArray*)children;


- (NSValue*)size;


- (NSValue*)position;


- (NSString*)role;



- (NSString*)subrole;


- (NSWindow*)window;


- (NSString*)customDescription;


- (id)value;


- (NSString*)title;


- (NSString*)help;

- (BOOL)isEnabled;


- (BOOL)isFocused;
- (BOOL)canBeFocused;


- (BOOL)focus;


- (void)didReceiveFocus;
- (void)valueDidChange;

#pragma mark -


- (void)invalidateChildren;



- (void)expire;
- (BOOL)isExpired;

#ifdef DEBUG
- (void)printHierarchy;
- (void)printHierarchyWithLevel:(unsigned)numSpaces;

- (void)sanityCheckChildren;
- (void)sanityCheckChildren:(NSArray *)theChildren;
#endif





- (BOOL)accessibilityIsIgnored;


- (id)accessibilityHitTest:(NSPoint)point;


- (id)accessibilityFocusedUIElement;



- (NSArray*)accessibilityAttributeNames;


- (id)accessibilityAttributeValue:(NSString*)attribute;

- (BOOL)accessibilityIsAttributeSettable:(NSString*)attribute;
- (void)accessibilitySetValue:(id)value forAttribute:(NSString*)attribute;

@end

