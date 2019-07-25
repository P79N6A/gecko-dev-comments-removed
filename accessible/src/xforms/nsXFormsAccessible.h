





































#ifndef _nsXFormsAccessible_H_
#define _nsXFormsAccessible_H_

#include "nsHyperTextAccessibleWrap.h"
#include "nsIXFormsUtilityService.h"

#define NS_NAMESPACE_XFORMS "http://www.w3.org/2002/xforms"




class nsXFormsAccessibleBase
{
public:
  nsXFormsAccessibleBase();

protected:
  
  enum { eAction_Click = 0 };

  
  static nsIXFormsUtilityService *sXFormsService;
};







class nsXFormsAccessible : public nsHyperTextAccessibleWrap,
                           public nsXFormsAccessibleBase
{
public:
  nsXFormsAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  

  
  NS_IMETHOD GetValue(nsAString& aValue);

  
  
  virtual void Description(nsString& aDescription);

  
  virtual nsresult GetNameInternal(nsAString& aName);

  
  
  virtual PRUint64 NativeState();

  
  
  virtual PRBool GetAllowsAnonChildAccessibles();

protected:
  
  
  nsresult GetBoundChildElementValue(const nsAString& aTagName,
                                     nsAString& aValue);

  
  
  
  
  
  
  
  void CacheSelectChildren(nsIDOMNode *aContainerNode = nsnull);
};














class nsXFormsContainerAccessible : public nsXFormsAccessible
{
public:
  nsXFormsContainerAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual PRUint32 NativeRole();

  
  
  virtual PRBool GetAllowsAnonChildAccessibles();
};






class nsXFormsEditableAccessible : public nsXFormsAccessible
{
public:
  nsXFormsEditableAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetAssociatedEditor(nsIEditor **aEditor);

  
  virtual PRUint64 NativeState();
};






class nsXFormsSelectableAccessible : public nsXFormsEditableAccessible
{
public:
  nsXFormsSelectableAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual bool IsSelect();
  virtual already_AddRefed<nsIArray> SelectedItems();
  virtual PRUint32 SelectedItemCount();
  virtual nsAccessible* GetSelectedItem(PRUint32 aIndex);
  virtual bool IsItemSelected(PRUint32 aIndex);
  virtual bool AddItemToSelection(PRUint32 aIndex);
  virtual bool RemoveItemFromSelection(PRUint32 aIndex);
  virtual bool SelectAll();
  virtual bool UnselectAll();

protected:
  nsIContent* GetItemByIndex(PRUint32* aIndex,
                             nsAccessible* aAccessible = nsnull);

  PRBool mIsSelect1Element;
};





class nsXFormsSelectableItemAccessible : public nsXFormsAccessible
{
public:
  nsXFormsSelectableItemAccessible(nsIContent *aContent,
                                   nsIWeakReference *aShell);

  NS_IMETHOD GetValue(nsAString& aValue);
  NS_IMETHOD GetNumActions(PRUint8 *aCount);
  NS_IMETHOD DoAction(PRUint8 aIndex);

protected:
  bool IsSelected();
};

#endif

