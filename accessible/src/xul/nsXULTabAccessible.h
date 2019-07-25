





































#ifndef _nsXULTabAccessible_H_
#define _nsXULTabAccessible_H_


#include "nsBaseWidgetAccessible.h"
#include "nsXULMenuAccessible.h"
#include "XULSelectControlAccessible.h"




class nsXULTabAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Switch = 0 };

  nsXULTabAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual Relation RelationByType(PRUint32 aType);

  
  virtual PRUint8 ActionCount();
};





class nsXULTabsAccessible : public XULSelectControlAccessible
{
public:
  nsXULTabsAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetValue(nsAString& _retval);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();

  
  virtual PRUint8 ActionCount();
};





class nsXULTabpanelsAccessible : public nsAccessibleWrap
{
public:
  nsXULTabpanelsAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual mozilla::a11y::role NativeRole();
};












class nsXULTabpanelAccessible : public nsAccessibleWrap
{
public:
  nsXULTabpanelAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual mozilla::a11y::role NativeRole();
  virtual Relation RelationByType(PRUint32 aType);
};

#endif

