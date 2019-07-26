




#include "AccessibleWrap.h"

#import <Cocoa/Cocoa.h>

#import "mozAccessibleProtocol.h"

@class mozRootAccessible;







inline id <mozAccessible>
GetObjectOrRepresentedView(id <mozAccessible> aObject)
{
  return [aObject hasRepresentedView] ? [aObject representedView] : aObject;
}

inline mozAccessible*
GetNativeFromGeckoAccessible(nsIAccessible* aAccessible)
{
  mozAccessible* native = nil;
  aAccessible->GetNativeInterface((void**)&native);
  return native;
}

@interface mozAccessible : NSObject <mozAccessible>
{
  


  mozilla::a11y::AccessibleWrap* mGeckoAccessible;
  
  


  NSMutableArray* mChildren;
  
  


  mozAccessible* mParent;

  


  mozilla::a11y::role        mRole;
}


- (id)initWithAccessible:(mozilla::a11y::AccessibleWrap*)geckoParent;


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




- (void)appendChild:(mozilla::a11y::Accessible*)aAccessible;



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

