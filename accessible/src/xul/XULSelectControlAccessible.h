






































#ifndef _XULSelectControlAccessible_H_
#define _XULSelectControlAccessible_H_

#include "nsAccessibleWrap.h"
#include "nsIDOMXULSelectCntrlEl.h"




class XULSelectControlAccessible : public nsAccessibleWrap
{
public:
  XULSelectControlAccessible(nsIContent* aContent, nsDocAccessible* aDoc);
  virtual ~XULSelectControlAccessible() {}

  
  virtual void Shutdown();

  
  virtual bool IsSelect();
  virtual already_AddRefed<nsIArray> SelectedItems();
  virtual PRUint32 SelectedItemCount();
  virtual nsAccessible* GetSelectedItem(PRUint32 aIndex);
  virtual bool IsItemSelected(PRUint32 aIndex);
  virtual bool AddItemToSelection(PRUint32 aIndex);
  virtual bool RemoveItemFromSelection(PRUint32 aIndex);
  virtual bool SelectAll();
  virtual bool UnselectAll();

  
  virtual nsAccessible* CurrentItem();
  virtual void SetCurrentItem(nsAccessible* aItem);

protected:
  
  
  nsCOMPtr<nsIDOMXULSelectControlElement> mSelectControl;
};

#endif

