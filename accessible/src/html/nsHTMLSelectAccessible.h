





































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

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

protected:

  
  virtual void CacheChildren();

  

  


  void CacheOptSiblings(nsIContent *aParentContent);
};




class nsHTMLSelectOptionAccessible : public nsHyperTextAccessibleWrap
{
public:
  enum { eAction_Select = 0 };  
  
  nsHTMLSelectOptionAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsHTMLSelectOptionAccessible() {}

  
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);

  nsIFrame*  GetBoundsFrame();
  static nsresult GetFocusedOptionNode(nsIDOMNode *aListNode, nsIDOMNode **aFocusedOptionNode);
  static void SelectionChangedIfOption(nsIContent *aPossibleOption);

private:
  
  




 
  nsIContent* GetSelectState(PRUint32* aState, PRUint32* aExtraState = nsnull);
};




class nsHTMLSelectOptGroupAccessible : public nsHTMLSelectOptionAccessible
{
public:

  nsHTMLSelectOptGroupAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsHTMLSelectOptGroupAccessible() {}

  
  NS_IMETHOD DoAction(PRUint8 index);  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

protected:
  
  virtual void CacheChildren();
};





class nsHTMLComboboxListAccessible;




class nsHTMLComboboxAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  nsHTMLComboboxAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsHTMLComboboxAccessible() {}

  
  NS_IMETHOD GetValue(nsAString& _retval);
  NS_IMETHOD GetDescription(nsAString& aDescription);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual nsresult Shutdown();

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

protected:
  
  virtual void CacheChildren();

  
  already_AddRefed<nsIAccessible> GetFocusedOptionAccessible();

private:
  nsRefPtr<nsHTMLComboboxListAccessible> mListAccessible;
};






class nsHTMLComboboxListAccessible : public nsHTMLSelectListAccessible
{
public:

  nsHTMLComboboxListAccessible(nsIAccessible *aParent, 
                               nsIDOMNode* aDOMNode, 
                               nsIWeakReference* aShell);
  virtual ~nsHTMLComboboxListAccessible() {}

  
  NS_IMETHOD GetUniqueID(void **aUniqueID);

  
  virtual nsIFrame* GetFrame();

  
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual void GetBoundsRect(nsRect& aBounds, nsIFrame** aBoundingFrame);
  virtual nsIAccessible* GetParent();
};

#endif
