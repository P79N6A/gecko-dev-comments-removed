





#ifndef mozilla_a11y_xpcAccessible_h_
#define mozilla_a11y_xpcAccessible_h_

#include "nsIAccessible.h"

class nsIAccessible;

namespace mozilla {
namespace a11y {

class Accessible;





class xpcAccessible : public nsIAccessible
{
public:
  
  NS_IMETHOD GetParent(nsIAccessible** aParent) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetNextSibling(nsIAccessible** aNextSibling) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetPreviousSibling(nsIAccessible** aPreviousSibling)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetFirstChild(nsIAccessible** aFirstChild) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetLastChild(nsIAccessible** aLastChild) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetChildCount(int32_t* aChildCount) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetChildAt(int32_t aChildIndex, nsIAccessible** aChild)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetChildren(nsIArray** aChildren) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetIndexInParent(int32_t* aIndexInParent) MOZ_FINAL MOZ_OVERRIDE;

  NS_IMETHOD GetDOMNode(nsIDOMNode** aDOMNode) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetDocument(nsIAccessibleDocument** aDocument) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetRootDocument(nsIAccessibleDocument** aRootDocument)
    MOZ_FINAL MOZ_OVERRIDE;

  NS_IMETHOD GetRole(uint32_t* aRole) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetState(uint32_t* aState, uint32_t* aExtraState)
    MOZ_FINAL MOZ_OVERRIDE;

  NS_IMETHOD GetDescription(nsAString& aDescription) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetName(nsAString& aName) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetLanguage(nsAString& aLanguage) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetValue(nsAString& aValue) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetHelp(nsAString& aHelp) MOZ_FINAL MOZ_OVERRIDE;

  NS_IMETHOD GetAccessKey(nsAString& aAccessKey) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetKeyboardShortcut(nsAString& aKeyBinding) MOZ_FINAL MOZ_OVERRIDE;

  NS_IMETHOD GetAttributes(nsIPersistentProperties** aAttributes)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetBounds(int32_t* aX, int32_t* aY,
                       int32_t* aWidth, int32_t* aHeight) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GroupPosition(int32_t* aGroupLevel, int32_t* aSimilarItemsInGroup,
                           int32_t* aPositionInGroup) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetRelationByType(uint32_t aType,
                               nsIAccessibleRelation** aRelation)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetRelations(nsIArray** aRelations) MOZ_FINAL MOZ_OVERRIDE;

  NS_IMETHOD GetFocusedChild(nsIAccessible** aChild) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetChildAtPoint(int32_t aX, int32_t aY,
                             nsIAccessible** aAccessible) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetDeepestChildAtPoint(int32_t aX, int32_t aY,
                                    nsIAccessible** aAccessible)
    MOZ_FINAL MOZ_OVERRIDE;

  NS_IMETHOD SetSelected(bool aSelect) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD ExtendSelection() MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD TakeSelection() MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD TakeFocus() MOZ_FINAL MOZ_OVERRIDE;

  NS_IMETHOD GetActionCount(uint8_t* aActionCount) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetActionDescription(uint8_t aIndex, nsAString& aDescription)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD DoAction(uint8_t aIndex) MOZ_FINAL MOZ_OVERRIDE;

  NS_IMETHOD ScrollTo(uint32_t aHow) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD ScrollToPoint(uint32_t aCoordinateType,
                           int32_t aX, int32_t aY) MOZ_FINAL MOZ_OVERRIDE;

protected:
  xpcAccessible() { }
  virtual ~xpcAccessible() {}

private:
  Accessible* Intl();

  xpcAccessible(const xpcAccessible&) = delete;
  xpcAccessible& operator =(const xpcAccessible&) = delete;
};

} 
} 

#endif
