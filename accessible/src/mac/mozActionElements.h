





































#import <Cocoa/Cocoa.h>
#import "mozAccessible.h"



@interface mozButtonAccessible : mozAccessible
- (void)click;
@end

@interface mozCheckboxAccessible : mozButtonAccessible

- (int)isChecked;
@end


@interface mozPopupButtonAccessible : mozButtonAccessible
@end
