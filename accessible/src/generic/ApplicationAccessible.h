






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

  
  NS_IMETHOD GetDOMNode(nsIDOMNode** aDOMNode);
  NS_IMETHOD GetDocument(nsIAccessibleDocument** aDocument);
  NS_IMETHOD GetRootDocument(nsIAccessibleDocument** aRootDocument);
  NS_IMETHOD ScrollTo(uint32_t aScrollType);
  NS_IMETHOD ScrollToPoint(uint32_t aCoordinateType, int32_t aX, int32_t aY);
  NS_IMETHOD GetLanguage(nsAString& aLanguage);
  NS_IMETHOD GetParent(nsIAccessible **aParent);
  NS_IMETHOD GetNextSibling(nsIAccessible **aNextSibling);
  NS_IMETHOD GetPreviousSibling(nsIAccessible **aPreviousSibling);
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
  NS_IMETHOD GetBounds(int32_t *aX, int32_t *aY,
                       int32_t *aWidth, int32_t *aHeight);
  NS_IMETHOD SetSelected(bool aIsSelected);
  NS_IMETHOD TakeSelection();
  NS_IMETHOD TakeFocus();
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString &aName);
  NS_IMETHOD GetActionDescription(uint8_t aIndex, nsAString &aDescription);
  NS_IMETHOD DoAction(uint8_t aIndex);

  
  NS_DECL_NSIACCESSIBLEAPPLICATION

  
  virtual void Init();
  virtual void Shutdown();

  
  virtual GroupPos GroupPosition();
  virtual ENameValueFlag Name(nsString& aName);
  virtual void ApplyARIAState(uint64_t* aState) const;
  virtual void Description(nsString& aDescription);
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();
  virtual uint64_t State();
  virtual uint64_t NativeState();
  virtual Relation RelationByType(uint32_t aRelType);

  virtual Accessible* ChildAtPoint(int32_t aX, int32_t aY,
                                   EWhichChildAtPoint aWhichChild);
  virtual Accessible* FocusedChild();

  virtual void InvalidateChildren();

  
  virtual uint8_t ActionCount();
  virtual KeyBinding AccessKey() const;

protected:

  
  virtual void CacheChildren();
  virtual Accessible* GetSiblingAtOffset(int32_t aOffset,
                                         nsresult *aError = nullptr) const;

private:
  nsCOMPtr<nsIXULAppInfo> mAppInfo;
};

} 
} 

#endif

