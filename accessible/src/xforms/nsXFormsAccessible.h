





































#ifndef _nsXFormsAccessible_H_
#define _nsXFormsAccessible_H_

#include "nsHyperTextAccessible.h"
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







class nsXFormsAccessible : public nsHyperTextAccessible,
                           public nsXFormsAccessibleBase
{
public:
  nsXFormsAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetValue(nsAString& aValue);

  
  
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);

  
  NS_IMETHOD GetName(nsAString& aName);

  
  NS_IMETHOD GetDescription(nsAString& aDescription);

  
  
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);

  
  
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren);

protected:
  
  
  nsresult GetBoundChildElementValue(const nsAString& aTagName,
                                     nsAString& aValue);

  
  
  
  
  
  
  
  void CacheSelectChildren(nsIDOMNode *aContainerNode = nsnull);
};














class nsXFormsContainerAccessible : public nsXFormsAccessible
{
public:
  nsXFormsContainerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetRole(PRUint32 *aRole);

  
  
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren);
};






class nsXFormsEditableAccessible : public nsXFormsAccessible
{
public:
  nsXFormsEditableAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell);

  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);

  NS_IMETHOD Init();
  NS_IMETHOD Shutdown();

protected:
  virtual void SetEditor(nsIEditor *aEditor);
  virtual already_AddRefed<nsIEditor> GetEditor();

private:
  nsCOMPtr<nsIEditor> mEditor;
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

