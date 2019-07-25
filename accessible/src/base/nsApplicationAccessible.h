









































#ifndef __NS_APPLICATION_ACCESSIBLE_H__
#define __NS_APPLICATION_ACCESSIBLE_H__

#include "nsAccessibleWrap.h"
#include "nsIAccessibleApplication.h"

#include "nsIMutableArray.h"
#include "nsIXULAppInfo.h"











class nsApplicationAccessible: public nsAccessibleWrap,
                               public nsIAccessibleApplication
{
public:

  nsApplicationAccessible();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_SCRIPTABLE NS_IMETHOD GetDOMNode(nsIDOMNode** aDOMNode);
  NS_SCRIPTABLE NS_IMETHOD GetDocument(nsIAccessibleDocument** aDocument);
  NS_SCRIPTABLE NS_IMETHOD GetRootDocument(nsIAccessibleDocument** aRootDocument);
  NS_SCRIPTABLE NS_IMETHOD GetInnerHTML(nsAString& aInnerHTML);
  NS_SCRIPTABLE NS_IMETHOD ScrollTo(PRUint32 aScrollType);
  NS_SCRIPTABLE NS_IMETHOD ScrollToPoint(PRUint32 aCoordinateType, PRInt32 aX, PRInt32 aY);
  NS_SCRIPTABLE NS_IMETHOD GetComputedStyleValue(const nsAString& aPseudoElt,
                                                 const nsAString& aPropertyName,
                                                 nsAString& aValue NS_OUTPARAM);
  NS_SCRIPTABLE NS_IMETHOD GetComputedStyleCSSValue(const nsAString& aPseudoElt,
                                                    const nsAString& aPropertyName,
                                                    nsIDOMCSSPrimitiveValue** aValue NS_OUTPARAM);
  NS_SCRIPTABLE NS_IMETHOD GetLanguage(nsAString& aLanguage);

  
  NS_IMETHOD GetParent(nsIAccessible **aParent);
  NS_IMETHOD GetNextSibling(nsIAccessible **aNextSibling);
  NS_IMETHOD GetPreviousSibling(nsIAccessible **aPreviousSibling);
  NS_IMETHOD GetName(nsAString &aName);
  NS_IMETHOD GetValue(nsAString &aValue);
  NS_IMETHOD GetAttributes(nsIPersistentProperties **aAttributes);
  NS_IMETHOD GroupPosition(PRInt32 *aGroupLevel, PRInt32 *aSimilarItemsInGroup,
                           PRInt32 *aPositionInGroup);
  NS_IMETHOD GetBounds(PRInt32 *aX, PRInt32 *aY,
                       PRInt32 *aWidth, PRInt32 *aHeight);
  NS_IMETHOD SetSelected(bool aIsSelected);
  NS_IMETHOD TakeSelection();
  NS_IMETHOD TakeFocus();
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString &aName);
  NS_IMETHOD GetActionDescription(PRUint8 aIndex, nsAString &aDescription);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  NS_DECL_NSIACCESSIBLEAPPLICATION

  
  virtual bool IsDefunct() const;
  virtual bool Init();
  virtual void Shutdown();
  virtual bool IsPrimaryForNode() const;

  
  virtual void ApplyARIAState(PRUint64* aState);
  virtual void Description(nsString& aDescription);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 State();
  virtual PRUint64 NativeState();
  virtual Relation RelationByType(PRUint32 aRelType);

  virtual nsAccessible* ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                     EWhichChildAtPoint aWhichChild);
  virtual nsAccessible* FocusedChild();

  virtual void InvalidateChildren();

  
  virtual PRUint8 ActionCount();
  virtual KeyBinding AccessKey() const;

protected:

  
  virtual void CacheChildren();
  virtual nsAccessible* GetSiblingAtOffset(PRInt32 aOffset,
                                           nsresult *aError = nsnull) const;

private:
  nsCOMPtr<nsIXULAppInfo> mAppInfo;
};

#endif

