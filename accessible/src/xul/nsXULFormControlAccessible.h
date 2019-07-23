






































#ifndef _nsXULFormControlAccessible_H_
#define _nsXULFormControlAccessible_H_


#include "nsAccessibleWrap.h"
#include "nsFormControlAccessible.h"
#include "nsXULSelectAccessible.h"
#include "nsHyperTextAccessible.h"

class nsXULButtonAccessible : public nsAccessibleWrap

{
public:
  enum { eAction_Click = 0 };
  nsXULButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *_retval);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  void CacheChildren();
};

class nsXULCheckboxAccessible : public nsFormControlAccessible
{
public:
  enum { eAction_Click = 0 };
  nsXULCheckboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetState(PRUint32 *_retval); 
};

class nsXULDropmarkerAccessible : public nsFormControlAccessible
{
public:
  enum { eAction_Click = 0 };
  nsXULDropmarkerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *_retval); 
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

private:
  PRBool DropmarkerOpen(PRBool aToggleOpen);
};

class nsXULGroupboxAccessible : public nsAccessibleWrap
{
public:
  nsXULGroupboxAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *_retval); 
  NS_IMETHOD GetName(nsAString& _retval);
};

class nsXULProgressMeterAccessible : public nsFormControlAccessible
{
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLEVALUE

public:
  nsXULProgressMeterAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *aRole); 
  NS_IMETHOD GetState(PRUint32 *aState); 
  NS_IMETHOD GetValue(nsAString &aValue);
};

class nsXULRadioButtonAccessible : public nsRadioButtonAccessible
{

public:
  nsXULRadioButtonAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetState(PRUint32 *aState);
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
};

class nsXULRadioGroupAccessible : public nsXULSelectableAccessible
{
public:
  nsXULRadioGroupAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *_retval); 
};

class nsXULStatusBarAccessible : public nsAccessibleWrap
{
public:
  nsXULStatusBarAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *_retval); 
};

class nsXULToolbarAccessible : public nsAccessibleWrap
{
public:
  nsXULToolbarAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *_retval); 
};

class nsXULToolbarSeparatorAccessible : public nsLeafAccessible
{
public:
  nsXULToolbarSeparatorAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_IMETHOD GetRole(PRUint32 *_retval); 
  NS_IMETHOD GetState(PRUint32 *_retval); 
};

class nsXULTextFieldAccessible : public nsHyperTextAccessible
{
public:
  enum { eAction_Click = 0 };

  NS_DECL_ISUPPORTS_INHERITED

  nsXULTextFieldAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  NS_IMETHOD Init(); 
  NS_IMETHOD Shutdown(); 
  NS_IMETHOD GetValue(nsAString& aValue);
  NS_IMETHOD GetState(PRUint32 *aState);
  NS_IMETHOD GetExtState(PRUint32 *aExtState);
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetNumActions(PRUint8 *_retval);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);
  NS_IMETHOD GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren);

protected:
  already_AddRefed<nsIDOMNode> GetInputField();

  
  virtual void SetEditor(nsIEditor *aEditor);
  virtual already_AddRefed<nsIEditor> GetEditor() { nsIEditor *editor = mEditor; NS_IF_ADDREF(editor); return editor; }
  void CheckForEditor();
  nsCOMPtr<nsIEditor> mEditor;
};


#endif  

