






































#ifndef __nsXULListboxAccessible_h__
#define __nsXULListboxAccessible_h__

#include "nsIAccessibleTable.h"

#include "nsCOMPtr.h"
#include "nsXULMenuAccessible.h"
#include "nsBaseWidgetAccessible.h"

class nsIWeakReference;





class nsXULColumnsAccessible : public nsAccessibleWrap
{
public:
  nsXULColumnsAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual PRUint32 NativeRole();
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};





class nsXULColumnItemAccessible : public nsLeafAccessible
{
public:
  nsXULColumnItemAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  virtual PRUint32 NativeRole();
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

  enum { eAction_Click = 0 };
};




class nsXULListboxAccessible : public nsXULSelectableAccessible,
                               public nsIAccessibleTable
{
public:
  nsXULListboxAccessible(nsIContent *aContent, nsIWeakReference *aShell);
  virtual ~nsXULListboxAccessible() {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETABLE

  
  NS_IMETHOD GetValue(nsAString& aValue);

  
  virtual PRUint32 NativeRole();
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

protected:
  PRBool IsMulticolumn();
};




class nsXULListitemAccessible : public nsXULMenuitemAccessible
{
public:
  enum { eAction_Click = 0 };

  NS_DECL_ISUPPORTS_INHERITED
  
  nsXULListitemAccessible(nsIContent *aContent, nsIWeakReference *aShell);
  virtual ~nsXULListitemAccessible() {}

  
  NS_IMETHOD GetActionName(PRUint8 index, nsAString& aName);
  
  NS_IMETHOD GetDescription(nsAString& aDesc) { return nsAccessibleWrap::GetDescription(aDesc); }

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual PRUint32 NativeRole();
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);
  virtual PRBool GetAllowsAnonChildAccessibles();

protected:
  


  nsAccessible *GetListAccessible();

private:
  PRBool mIsCheckbox;
};




class nsXULListCellAccessible : public nsHyperTextAccessibleWrap,
                                public nsIAccessibleTableCell
{
public:
  nsXULListCellAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIACCESSIBLETABLECELL

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual PRUint32 NativeRole();
};

#endif
