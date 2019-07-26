






#ifndef mozilla_a11y_ApplicationAccessible_h__
#define mozilla_a11y_ApplicationAccessible_h__

#include "AccessibleWrap.h"
#include "nsIAccessibleApplication.h"

#include "nsIMutableArray.h"
#include "nsIXULAppInfo.h"

namespace mozilla {
namespace a11y {











class ApplicationAccessible : public AccessibleWrap,
                             public nsIAccessibleApplication
{
public:

  ApplicationAccessible();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_SCRIPTABLE NS_IMETHOD GetDOMNode(nsIDOMNode** aDOMNode);
  NS_SCRIPTABLE NS_IMETHOD GetDocument(nsIAccessibleDocument** aDocument);
  NS_SCRIPTABLE NS_IMETHOD GetRootDocument(nsIAccessibleDocument** aRootDocument);
  NS_SCRIPTABLE NS_IMETHOD ScrollTo(PRUint32 aScrollType);
  NS_SCRIPTABLE NS_IMETHOD ScrollToPoint(PRUint32 aCoordinateType, PRInt32 aX, PRInt32 aY);
  NS_SCRIPTABLE NS_IMETHOD GetLanguage(nsAString& aLanguage);
  NS_IMETHOD GetParent(nsIAccessible **aParent);
  NS_IMETHOD GetNextSibling(nsIAccessible **aNextSibling);
  NS_IMETHOD GetPreviousSibling(nsIAccessible **aPreviousSibling);
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
  NS_IMETHOD GetBounds(PRInt32 *aX, PRInt32 *aY,
                       PRInt32 *aWidth, PRInt32 *aHeight);
  NS_IMETHOD SetSelected(bool aIsSelected);
  NS_IMETHOD TakeSelection();
  NS_IMETHOD TakeFocus();
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString &aName);
  NS_IMETHOD GetActionDescription(PRUint8 aIndex, nsAString &aDescription);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  NS_DECL_NSIACCESSIBLEAPPLICATION

  
  virtual bool Init();
  virtual void Shutdown();
  virtual bool IsPrimaryForNode() const;

  
  virtual GroupPos GroupPosition();
  virtual ENameValueFlag Name(nsString& aName);
  virtual void ApplyARIAState(PRUint64* aState) const;
  virtual void Description(nsString& aDescription);
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 State();
  virtual PRUint64 NativeState();
  virtual Relation RelationByType(PRUint32 aRelType);

  virtual Accessible* ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   EWhichChildAtPoint aWhichChild);
  virtual Accessible* FocusedChild();

  virtual void InvalidateChildren();

  
  virtual PRUint8 ActionCount();
  virtual KeyBinding AccessKey() const;

protected:

  
  virtual void CacheChildren();
  virtual Accessible* GetSiblingAtOffset(PRInt32 aOffset,
                                         nsresult *aError = nsnull) const;

private:
  nsCOMPtr<nsIXULAppInfo> mAppInfo;
};

} 
} 

#endif

