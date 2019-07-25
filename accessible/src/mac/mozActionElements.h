





































#import <Cocoa/Cocoa.h>
#import "mozAccessible.h"



@interface mozButtonAccessible : mozAccessible
- (void)click;
- (BOOL)isTab;
@end

@interface mozCheckboxAccessible : mozButtonAccessible

- (int)isChecked;
@end


@interface mozPopupButtonAccessible : mozButtonAccessible
@end


@interface mozTabsAccessible : mozAccessible
{
  NSMutableArray* mTabs;
}
-(id)tabs;
@end
