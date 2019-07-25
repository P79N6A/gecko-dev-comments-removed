#import "mozAccessible.h"

#import "nsHyperTextAccessible.h"

@interface mozTextAccessible : mozAccessible
{
  
  
  nsHyperTextAccessible     *mGeckoTextAccessible;         
  nsIAccessibleEditableText *mGeckoEditableTextAccessible; 
}
@end



@interface mozComboboxAccessible : mozTextAccessible

- (void)confirm;


- (void)showMenu;
@end
