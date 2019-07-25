





#ifndef mozilla_a11y_XULSelectControlAccessible_h__
#define mozilla_a11y_XULSelectControlAccessible_h__

#include "AccessibleWrap.h"
#include "nsIDOMXULSelectCntrlEl.h"

namespace mozilla {
namespace a11y {




class XULSelectControlAccessible : public AccessibleWrap
{
public:
  XULSelectControlAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~XULSelectControlAccessible() {}

  
  virtual void Shutdown();

  
  virtual bool IsSelect();
  virtual already_AddRefed<nsIArray> SelectedItems();
  virtual uint32_t SelectedItemCount();
  virtual Accessible* GetSelectedItem(uint32_t aIndex);
  virtual bool IsItemSelected(uint32_t aIndex);
  virtual bool AddItemToSelection(uint32_t aIndex);
  virtual bool RemoveItemFromSelection(uint32_t aIndex);
  virtual bool SelectAll();
  virtual bool UnselectAll();

  
  virtual Accessible* CurrentItem();
  virtual void SetCurrentItem(Accessible* aItem);

protected:
  
  
  nsCOMPtr<nsIDOMXULSelectControlElement> mSelectControl;
};

} 
} 

#endif

