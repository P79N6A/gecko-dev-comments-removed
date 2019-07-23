





































#ifndef _nsHTMLFormControlAccessible_H_
#define _nsHTMLFormControlAccessible_H_

#include "nsFormControlAccessible.h"
#include "nsHyperTextAccessible.h"

class nsHTMLCheckboxAccessible : public nsFormControlAccessible
{

public:
  enum { eAction_Click = 0 };

  nsHTMLCheckboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetState(PRUint32 *aState); 
};

class nsHTMLRadioButtonAccessible : public nsRadioButtonAccessible
{

public:
  nsHTMLRadioButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  NS_IMETHOD GetState(PRUint32 *_retval);
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
};

class nsHTMLButtonAccessible : public nsHyperTextAccessible
{

public:
  enum { eAction_Click = 0 };

  nsHTMLButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *_retval); 
  NS_IMETHOD GetName(nsAString& _retval); 
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
};

class nsHTML4ButtonAccessible : public nsHyperTextAccessible
{

public:
  enum { eAction_Click = 0 };

  nsHTML4ButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *_retval);
  NS_IMETHOD GetName(nsAString& aName) { aName.Truncate(); return GetHTMLName(aName, PR_TRUE); }
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
};

class nsHTMLTextFieldAccessible : public nsHyperTextAccessible
{

public:
  enum { eAction_Click = 0 };

  NS_DECL_ISUPPORTS_INHERITED

  nsHTMLTextFieldAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  NS_IMETHOD Init(); 
  NS_IMETHOD Shutdown(); 
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetValue(nsAString& _retval); 
  NS_IMETHOD GetState(PRUint32 *_retval);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetExtState(PRUint32 *aExtState); 

protected:
  
  virtual void SetEditor(nsIEditor *aEditor);
  virtual already_AddRefed<nsIEditor> GetEditor() { nsIEditor *editor = mEditor; NS_IF_ADDREF(editor); return editor; }
  void CheckForEditor();
  nsCOMPtr<nsIEditor> mEditor;
};

class nsHTMLGroupboxAccessible : public nsAccessibleWrap
{
public:
  nsHTMLGroupboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *_retval);
  NS_IMETHOD GetName(nsAString& _retval);
  void CacheChildren();
};

#endif  
