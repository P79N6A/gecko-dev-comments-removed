





































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
  nsXFormsAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  

  
  NS_IMETHOD GetValue(nsAString& aValue);

  
  NS_IMETHOD GetDescription(nsAString& aDescription);

  

  
  virtual nsresult GetNameInternal(nsAString& aName);

  
  
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  
  
  virtual PRBool GetAllowsAnonChildAccessibles();

protected:
  
  
  nsresult GetBoundChildElementValue(const nsAString& aTagName,
                                     nsAString& aValue);

  
  
  
  
  
  
  
  void CacheSelectChildren(nsIDOMNode *aContainerNode = nsnull);
};














class nsXFormsContainerAccessible : public nsXFormsAccessible
{
public:
  nsXFormsContainerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);

  
  
  virtual PRBool GetAllowsAnonChildAccessibles();
};






class nsXFormsEditableAccessible : public nsXFormsAccessible
{
public:
  nsXFormsEditableAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  
  NS_IMETHOD GetAssociatedEditor(nsIEditor **aEditor);

  
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};






class nsXFormsSelectableAccessible : public nsXFormsEditableAccessible
{
public:
  nsXFormsSelectableAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLESELECTABLE

protected:
  already_AddRefed<nsIDOMNode> GetItemByIndex(PRInt32 *aIndex,
                                              nsIAccessible *aAccessible = nsnull);
  PRBool mIsSelect1Element;
};





class nsXFormsSelectableItemAccessible : public nsXFormsAccessible
{
public:
  nsXFormsSelectableItemAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  NS_IMETHOD GetValue(nsAString& aValue);
  NS_IMETHOD GetNumActions(PRUint8 *aCount);
  NS_IMETHOD DoAction(PRUint8 aIndex);

protected:
  PRBool IsItemSelected();
};

#endif

