




#include "nsAccessibleWrap.h"

#import <Cocoa/Cocoa.h>

#import "mozAccessibleProtocol.h"

@class mozRootAccessible;







inline id <mozAccessible>
GetObjectOrRepresentedView(id <mozAccessible> aObject)
{
  return [aObject hasRepresentedView] ? [aObject representedView] : aObject;
}

@interface mozAccessible : NSObject <mozAccessible>
{
  


  nsAccessibleWrap* mGeckoAccessible;
  
  


  NSMutableArray* mChildren;
  
  


  mozAccessible* mParent;
  
  





  BOOL mIsExpired;
  
  


  mozilla::a11y::role        mRole;
}


- (id)initWithAccessible:(nsAccessibleWrap*)geckoParent;


- (id <mozAccessible>)parent;


- (NSArray*)children;


- (NSValue*)size;


- (NSValue*)position;


- (NSString*)role;



- (NSString*)subrole;


- (NSString*)roleDescription;


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
- (void)selectedTextDidChange;

#pragma mark -


- (void)invalidateChildren;




- (void)appendChild:(nsAccessible*)aAccessible;



- (void)expire;
- (BOOL)isExpired;

#ifdef DEBUG
- (void)printHierarchy;
- (void)printHierarchyWithLevel:(unsigned)numSpaces;

- (void)sanityCheckChildren;
- (void)sanityCheckChildren:(NSArray*)theChildren;
#endif





- (BOOL)accessibilityIsIgnored;


- (id)accessibilityHitTest:(NSPoint)point;


- (id)accessibilityFocusedUIElement;



- (NSArray*)accessibilityAttributeNames;


- (id)accessibilityAttributeValue:(NSString*)attribute;

- (BOOL)accessibilityIsAttributeSettable:(NSString*)attribute;
- (void)accessibilitySetValue:(id)value forAttribute:(NSString*)attribute;

@end

