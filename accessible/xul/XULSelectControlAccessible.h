





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

  
  virtual void Shutdown() MOZ_OVERRIDE;

  
  virtual void SelectedItems(nsTArray<Accessible*>* aItems) MOZ_OVERRIDE;
  virtual uint32_t SelectedItemCount() MOZ_OVERRIDE;
  virtual Accessible* GetSelectedItem(uint32_t aIndex) MOZ_OVERRIDE;
  virtual bool IsItemSelected(uint32_t aIndex) MOZ_OVERRIDE;
  virtual bool AddItemToSelection(uint32_t aIndex) MOZ_OVERRIDE;
  virtual bool RemoveItemFromSelection(uint32_t aIndex) MOZ_OVERRIDE;
  virtual bool SelectAll() MOZ_OVERRIDE;
  virtual bool UnselectAll() MOZ_OVERRIDE;

  
  virtual Accessible* CurrentItem() MOZ_OVERRIDE;
  virtual void SetCurrentItem(Accessible* aItem) MOZ_OVERRIDE;

protected:
  
  
  nsCOMPtr<nsIDOMXULSelectControlElement> mSelectControl;
};

} 
} 

#endif

