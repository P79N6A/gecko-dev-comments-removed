




#import <Cocoa/Cocoa.h>
#import "mozAccessible.h"



@interface mozButtonAccessible : mozAccessible
 {
 }
- (BOOL)hasPopup;
- (void)click;
- (BOOL)isTab;
@end

@interface mozCheckboxAccessible : mozButtonAccessible

- (int)isChecked;
@end


@interface mozTabsAccessible : mozAccessible
{
  NSMutableArray* mTabs;
}
-(id)tabs;
@end




@interface mozPaneAccessible : mozAccessible

@end
