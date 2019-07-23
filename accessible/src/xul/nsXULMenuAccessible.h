





































#ifndef _nsXULMenuAccessible_H_
#define _nsXULMenuAccessible_H_

#include "nsAccessibleWrap.h"
#include "nsAccessibleTreeWalker.h"
#include "nsIAccessibleSelectable.h"
#include "nsIDOMXULSelectCntrlEl.h"




class nsXULSelectableAccessible : public nsAccessibleWrap
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLESELECTABLE

  nsXULSelectableAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsXULSelectableAccessible() {}
  NS_IMETHOD Shutdown();

protected:
  nsresult ChangeSelection(PRInt32 aIndex, PRUint8 aMethod, PRBool *aSelState);
  nsresult AppendFlatStringFromSubtree(nsIContent *aContent, nsAString *aFlatString)
    { return NS_OK; }  

  
  
  nsCOMPtr<nsIDOMXULSelectControlElement> mSelectControl;
};




class nsXULMenuitemAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  nsXULMenuitemAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD Init();
  NS_IMETHOD GetName(nsAString& _retval); 
  NS_IMETHOD GetDescription(nsAString& aDescription);
  NS_IMETHOD GetKeyboardShortcut(nsAString& _retval);
  NS_IMETHOD GetDefaultKeyBinding(nsAString& aKeyBinding);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren);
};

class nsXULMenuSeparatorAccessible : public nsXULMenuitemAccessible
{
public:
  nsXULMenuSeparatorAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetName(nsAString& _retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
};

class nsXULMenupopupAccessible : public nsXULSelectableAccessible
{
public:
  nsXULMenupopupAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetName(nsAString& _retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  static already_AddRefed<nsIDOMNode> FindInNodeList(nsIDOMNodeList *aNodeList,
                                                     nsIAtom *aAtom, PRUint32 aNameSpaceID);
  static void GenerateMenu(nsIDOMNode *aNode);
};

class nsXULMenubarAccessible : public nsAccessibleWrap
{
public:
  nsXULMenubarAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell);
  NS_IMETHOD GetName(nsAString& _retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
};

#endif  
