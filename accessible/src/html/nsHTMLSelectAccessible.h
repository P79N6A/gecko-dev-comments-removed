





































#ifndef __nsHTMLSelectAccessible_h__
#define __nsHTMLSelectAccessible_h__

#include "nsIAccessibleSelectable.h"
#include "nsAccessibilityAtoms.h"
#include "nsHTMLFormControlAccessible.h"
#include "nsIDOMHTMLOptionsCollection.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMNode.h"
#include "nsIAccessibilityService.h"
#include "nsAccessibleTreeWalker.h"

class nsIMutableArray;

























class nsHTMLSelectableAccessible : public nsAccessibleWrap
{
public:

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLESELECTABLE

  nsHTMLSelectableAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsHTMLSelectableAccessible() {}

  NS_IMETHOD GetName(nsAString &aName) { return GetHTMLName(aName, PR_FALSE); }

protected:

  NS_IMETHOD ChangeSelection(PRInt32 aIndex, PRUint8 aMethod, PRBool *aSelState);

  class iterator 
  {
  protected:
    PRUint32 mLength;
    PRUint32 mIndex;
    PRInt32 mSelCount;
    nsCOMPtr<nsIDOMHTMLOptionsCollection> mOptions;
    nsCOMPtr<nsIDOMHTMLOptionElement> mOption;
    nsCOMPtr<nsIWeakReference> mWeakShell;
    nsHTMLSelectableAccessible *mParentSelect;

  public:
    void Shutdown();
    iterator(nsHTMLSelectableAccessible *aParent, nsIWeakReference *aWeakShell);

    void CalcSelectionCount(PRInt32 *aSelectionCount);
    void Select(PRBool aSelect);
    void AddAccessibleIfSelected(nsIAccessibilityService *aAccService, 
                                 nsIMutableArray *aSelectedAccessibles, 
                                 nsPresContext *aContext);
    PRBool GetAccessibleIfSelected(PRInt32 aIndex, nsIAccessibilityService *aAccService, nsPresContext *aContext, nsIAccessible **_retval);

    PRBool Advance();
  };

  friend class iterator;
};




class nsHTMLSelectListAccessible : public nsHTMLSelectableAccessible
{
public:
  
  nsHTMLSelectListAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsHTMLSelectListAccessible() {}

  
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  void CacheChildren();

protected:
  already_AddRefed<nsIAccessible>
    AccessibleForOption(nsIAccessibilityService *aAccService,
                        nsIContent *aContent,
                        nsIAccessible *aLastGoodAccessible,
                        PRInt32 *aChildCount);
  already_AddRefed<nsIAccessible>
    CacheOptSiblings(nsIAccessibilityService *aAccService,
                     nsIContent *aParentContent,
                     nsIAccessible *aLastGoodAccessible,
                     PRInt32 *aChildCount);
};




class nsHTMLSelectOptionAccessible : public nsHyperTextAccessible
{
public:
  enum { eAction_Select = 0 };  
  
  nsHTMLSelectOptionAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsHTMLSelectOptionAccessible() {}

  
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetName(nsAString& aName);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

  nsIFrame*  GetBoundsFrame();
  static nsresult GetFocusedOptionNode(nsIDOMNode *aListNode, nsIDOMNode **aFocusedOptionNode);
  static void SelectionChangedIfOption(nsIContent *aPossibleOption);
};




class nsHTMLSelectOptGroupAccessible : public nsHTMLSelectOptionAccessible
{
public:

  nsHTMLSelectOptGroupAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsHTMLSelectOptGroupAccessible() {}

  
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD DoAction(PRUint8 index);  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
};








class nsHTMLComboboxAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  nsHTMLComboboxAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsHTMLComboboxAccessible() {}

  
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetValue(nsAString& _retval);
  NS_IMETHOD GetDescription(nsAString& aDescription);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  void CacheChildren();

protected:
  already_AddRefed<nsIAccessible> GetFocusedOptionAccessible();
};

#ifdef COMBO_BOX_WITH_THREE_CHILDREN




class nsHTMLComboboxTextFieldAccessible  : public nsHTMLTextFieldAccessible
{
public:
  
  nsHTMLComboboxTextFieldAccessible(nsIAccessible* aParent, nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsHTMLComboboxTextFieldAccessible() {}

  
  NS_IMETHOD GetUniqueID(void **aUniqueID);

  virtual void GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame);

protected:
  void CacheChildren();
};





class nsHTMLComboboxButtonAccessible  : public nsLeafAccessible
{
public:
  enum { eAction_Click = 0 };

  nsHTMLComboboxButtonAccessible(nsIAccessible* aParent, nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsHTMLComboboxButtonAccessible() {}

  
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD GetParent(nsIAccessible **_retval);
  NS_IMETHOD GetName(nsAString& _retval);
  NS_IMETHOD GetRole(PRUint32 *_retval);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetUniqueID(void **aUniqueID);

  virtual void GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame);
};
#endif






class nsHTMLComboboxListAccessible : public nsHTMLSelectListAccessible
{
public:

  nsHTMLComboboxListAccessible(nsIAccessible *aParent, 
                               nsIDOMNode* aDOMNode, 
                               nsIFrame *aListFrame,
                               nsIWeakReference* aShell);
  virtual ~nsHTMLComboboxListAccessible() {}

  
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetParent(nsIAccessible **aParent);
  NS_IMETHOD GetUniqueID(void **aUniqueID);

  
  NS_IMETHOD_(nsIFrame *) GetFrame(void);

  virtual void GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame);

protected:
  nsIFrame *mListFrame;
};

#endif
