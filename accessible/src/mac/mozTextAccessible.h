#import "mozAccessible.h"

#import "nsIAccessibleText.h"
#import "nsIAccessibleEditableText.h"

@interface mozTextAccessible : mozAccessible
{
  
  
  nsIAccessibleText         *mGeckoTextAccessible;         
  nsIAccessibleEditableText *mGeckoEditableTextAccessible; 
}
@end



@interface mozComboboxAccessible : mozTextAccessible

- (void)confirm;


- (void)showMenu;
@end
