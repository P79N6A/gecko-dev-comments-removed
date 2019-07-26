



#import "mozAccessible.h"

#import "HyperTextAccessible.h"

@interface mozTextAccessible : mozAccessible
{
  
  
  mozilla::a11y::HyperTextAccessible* mGeckoTextAccessible; 
  nsIAccessibleEditableText *mGeckoEditableTextAccessible; 
}
@end

@interface mozTextLeafAccessible : mozAccessible
{
}
@end
