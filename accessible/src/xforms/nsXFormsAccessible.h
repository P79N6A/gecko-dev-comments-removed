




#ifndef _nsXFormsAccessible_H_
#define _nsXFormsAccessible_H_

#include "HyperTextAccessibleWrap.h"
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







class nsXFormsAccessible : public HyperTextAccessibleWrap,
                           public nsXFormsAccessibleBase
{
public:
  nsXFormsAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  
  virtual void Description(nsString& aDescription);

  
  virtual void Value(nsString& aValue);

  
  virtual nsresult GetNameInternal(nsAString& aName);

  
  
  virtual uint64_t NativeState();
  virtual bool NativelyUnavailable() const;

  
  
  virtual bool CanHaveAnonChildren();

protected:
  
  
  nsresult GetBoundChildElementValue(const nsAString& aTagName,
                                     nsAString& aValue);

  
  
  
  
  
  
  
  void CacheSelectChildren(nsIDOMNode *aContainerNode = nullptr);
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

  
  virtual uint64_t NativeState();
};






class nsXFormsSelectableAccessible : public nsXFormsEditableAccessible
{
public:
  nsXFormsSelectableAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual bool IsSelect();
  virtual already_AddRefed<nsIArray> SelectedItems();
  virtual uint32_t SelectedItemCount();
  virtual Accessible* GetSelectedItem(uint32_t aIndex);
  virtual bool IsItemSelected(uint32_t aIndex);
  virtual bool AddItemToSelection(uint32_t aIndex);
  virtual bool RemoveItemFromSelection(uint32_t aIndex);
  virtual bool SelectAll();
  virtual bool UnselectAll();

protected:
  nsIContent* GetItemByIndex(uint32_t* aIndex,
                             Accessible* aAccessible = nullptr);

  bool mIsSelect1Element;
};





class nsXFormsSelectableItemAccessible : public nsXFormsAccessible
{
public:
  nsXFormsSelectableItemAccessible(nsIContent* aContent,
                                   DocAccessible* aDoc);

  NS_IMETHOD DoAction(uint8_t aIndex);

  
  virtual void Value(nsString& aValue);

  
  virtual uint8_t ActionCount();

protected:
  bool IsSelected();
};

#endif

