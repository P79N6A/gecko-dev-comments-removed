





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
  
  NS_IMETHOD GetSelectedItems(nsIArray** aSelectedItems) final override;
  NS_IMETHOD GetSelectedItemCount(uint32_t* aSelectedItemCount)
    final override;
  NS_IMETHOD GetSelectedItemAt(uint32_t aIndex, nsIAccessible** aItem)
    final override;
  NS_IMETHOD IsItemSelected(uint32_t aIndex, bool* aIsSelected)
    final override;
  NS_IMETHOD AddItemToSelection(uint32_t aIndex) final override;
  NS_IMETHOD RemoveItemFromSelection(uint32_t aIndex) final override;
  NS_IMETHOD SelectAll(bool* aIsMultiSelect) final override;
  NS_IMETHOD UnselectAll() final override;

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
