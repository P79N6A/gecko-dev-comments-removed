





































#ifndef _nsHTMLFormControlAccessible_H_
#define _nsHTMLFormControlAccessible_H_

#include "nsFormControlAccessible.h"
#include "nsHyperTextAccessibleWrap.h"

class nsHTMLCheckboxAccessible : public nsFormControlAccessible
{

public:
  enum { eAction_Click = 0 };

  nsHTMLCheckboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsHTMLRadioButtonAccessible : public nsRadioButtonAccessible
{

public:
  nsHTMLRadioButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsHTMLButtonAccessible : public nsHyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  nsHTMLButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsHTML4ButtonAccessible : public nsHyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  nsHTML4ButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsHTMLTextFieldAccessible : public nsHyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  nsHTMLTextFieldAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetValue(nsAString& _retval); 
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  NS_IMETHOD GetAssociatedEditor(nsIEditor **aEditor);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsHTMLGroupboxAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLGroupboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual nsresult GetRoleInternal(PRUint32 *aRole);

protected:
  nsIContent* GetLegend();
};

class nsHTMLLegendAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLLegendAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation **aRelation);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};

#endif  
