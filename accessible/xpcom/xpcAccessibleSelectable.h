





#ifndef mozilla_a11y_xpcAccessibleSelectable_h_
#define mozilla_a11y_xpcAccessibleSelectable_h_

#include "nsIAccessibleSelectable.h"

class nsIAccessible;
class nsIArray;

namespace mozilla {
namespace a11y {

class Accessible;





class xpcAccessibleSelectable : public nsIAccessibleSelectable
{
public:
  
  NS_IMETHOD GetSelectedItems(nsIArray** aSelectedItems) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetSelectedItemCount(uint32_t* aSelectedItemCount)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetSelectedItemAt(uint32_t aIndex, nsIAccessible** aItem)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD IsItemSelected(uint32_t aIndex, bool* aIsSelected)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD AddItemToSelection(uint32_t aIndex) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD RemoveItemFromSelection(uint32_t aIndex) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD SelectAll(bool* aIsMultiSelect) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD UnselectAll() MOZ_FINAL MOZ_OVERRIDE;

protected:
  xpcAccessibleSelectable() { }
  virtual ~xpcAccessibleSelectable() {}

private:
  xpcAccessibleSelectable(const xpcAccessibleSelectable&) = delete;
  xpcAccessibleSelectable& operator =(const xpcAccessibleSelectable&) = delete;

  Accessible* Intl();
};

} 
} 

#endif
