




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
  nsXFormsAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  
  virtual void Description(nsString& aDescription);

  
  virtual void Value(nsString& aValue);

  
  virtual nsresult GetNameInternal(nsAString& aName);

  
  
  virtual PRUint64 NativeState();

  
  
  virtual bool CanHaveAnonChildren();

protected:
  
  
  nsresult GetBoundChildElementValue(const nsAString& aTagName,
                                     nsAString& aValue);

  
  
  
  
  
  
  
  void CacheSelectChildren(nsIDOMNode *aContainerNode = nsnull);
};














class nsXFormsContainerAccessible : public nsXFormsAccessible
{
public:
  nsXFormsContainerAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();

  
  
  virtual bool CanHaveAnonChildren();
};






class nsXFormsEditableAccessible : public nsXFormsAccessible
{
public:
  nsXFormsEditableAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual already_AddRefed<nsIEditor> GetEditor() const;

  
  virtual PRUint64 NativeState();
};






class nsXFormsSelectableAccessible : public nsXFormsEditableAccessible
{
public:
  nsXFormsSelectableAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual bool IsSelect();
  virtual already_AddRefed<nsIArray> SelectedItems();
  virtual PRUint32 SelectedItemCount();
  virtual Accessible* GetSelectedItem(PRUint32 aIndex);
  virtual bool IsItemSelected(PRUint32 aIndex);
  virtual bool AddItemToSelection(PRUint32 aIndex);
  virtual bool RemoveItemFromSelection(PRUint32 aIndex);
  virtual bool SelectAll();
  virtual bool UnselectAll();

protected:
  nsIContent* GetItemByIndex(PRUint32* aIndex,
                             Accessible* aAccessible = nsnull);

  bool mIsSelect1Element;
};





class nsXFormsSelectableItemAccessible : public nsXFormsAccessible
{
public:
  nsXFormsSelectableItemAccessible(nsIContent* aContent,
                                   DocAccessible* aDoc);

  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual void Value(nsString& aValue);

  
  virtual PRUint8 ActionCount();

protected:
  bool IsSelected();
};

#endif

