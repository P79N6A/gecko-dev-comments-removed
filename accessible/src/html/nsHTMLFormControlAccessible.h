





































#ifndef _nsHTMLFormControlAccessible_H_
#define _nsHTMLFormControlAccessible_H_

#include "nsFormControlAccessible.h"
#include "nsHyperTextAccessibleWrap.h"

class nsHTMLCheckboxAccessible : public nsFormControlAccessible
{

public:
  enum { eAction_Click = 0 };

  nsHTMLCheckboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
};

class nsHTMLRadioButtonAccessible : public nsRadioButtonAccessible
{

public:
  nsHTMLRadioButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
};

class nsHTMLButtonAccessible : public nsHyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  nsHTMLButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetName(nsAString& _retval); 
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
};

class nsHTML4ButtonAccessible : public nsHyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  nsHTML4ButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetName(nsAString& aName) { aName.Truncate(); return GetHTMLName(aName, PR_TRUE); }
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
};

class nsHTMLTextFieldAccessible : public nsHyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  NS_DECL_ISUPPORTS_INHERITED

  nsHTMLTextFieldAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  NS_IMETHOD Init(); 
  NS_IMETHOD Shutdown(); 
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetName(nsAString& aName); 
  NS_IMETHOD GetValue(nsAString& _retval); 
  NS_IMETHOD GetState(PRUint32 *aState, PRUint32 *aExtraState);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

protected:
  
  virtual void SetEditor(nsIEditor *aEditor);
  virtual already_AddRefed<nsIEditor> GetEditor() { nsIEditor *editor = mEditor; NS_IF_ADDREF(editor); return editor; }
  void CheckForEditor();
  nsCOMPtr<nsIEditor> mEditor;
};

class nsHTMLGroupboxAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLGroupboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *aRole); 
  NS_IMETHOD GetName(nsAString& aName);
  NS_IMETHOD GetAccessibleRelated(PRUint32 aRelationType, nsIAccessible **aRelated);
protected:
  nsIContent* GetLegend();
};

class nsHTMLLegendAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLLegendAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetAccessibleRelated(PRUint32 aRelationType, nsIAccessible **aRelated);
  NS_IMETHOD GetRole(PRUint32 *aRole) { *aRole = nsIAccessibleRole::ROLE_LABEL; return NS_OK; }
};

#endif  
