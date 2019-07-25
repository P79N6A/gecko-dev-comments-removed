



#import "mozAccessible.h"

#import "HyperTextAccessible.h"

@interface mozTextAccessible : mozAccessible
{
  
  
  HyperTextAccessible *mGeckoTextAccessible; 
  nsIAccessibleEditableText *mGeckoEditableTextAccessible; 
}
@end

@interface mozTextLeafAccessible : mozAccessible
{
}
@end
