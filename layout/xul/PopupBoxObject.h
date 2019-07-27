





#ifndef mozilla_dom_PopupBoxObject_h
#define mozilla_dom_PopupBoxObject_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/BoxObject.h"
#include "nsString.h"

struct JSContext;
class nsPopupSetFrame;

namespace mozilla {
namespace dom {

class DOMRect;
class Element;
class Event;

class PopupBoxObject MOZ_FINAL : public BoxObject
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  
  static const uint32_t ROLLUP_DEFAULT = 0;   
  static const uint32_t ROLLUP_CONSUME = 1;   
  static const uint32_t ROLLUP_NO_CONSUME = 2; 

  PopupBoxObject();

  nsIContent* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  void ShowPopup(Element* aAnchorElement,
                 Element& aPopupElement,
                 int32_t aXPos,
                 int32_t aYPos,
                 const nsAString& aPopupType,
                 const nsAString& aAnchorAlignment,
                 const nsAString& aPopupAlignment);

  void HidePopup(bool aCancel);

  bool AutoPosition();

  void SetAutoPosition(bool aShouldAutoPosition);

  void EnableKeyboardNavigator(bool aEnableKeyboardNavigator);

  void EnableRollup(bool aShouldRollup);

  void SetConsumeRollupEvent(uint32_t aConsume);

  void SizeTo(int32_t aWidth, int32_t aHeight);

  void MoveTo(int32_t aLeft, int32_t aTop);

  void OpenPopup(Element* aAnchorElement,
                 const nsAString& aPosition,
                 int32_t aXPos,
                 int32_t aYPos,
                 bool aIsContextMenu, bool aAttributesOverride,
                 Event* aTriggerEvent);

  void OpenPopupAtScreen(int32_t aXPos,
                         int32_t aYPos,
                         bool aIsContextMenu,
                         Event* aTriggerEvent);

  void GetPopupState(nsString& aState);

  nsINode* GetTriggerNode() const;

  Element* GetAnchorNode() const;

  already_AddRefed<DOMRect> GetOuterScreenRect();

  void MoveToAnchor(Element* aAnchorElement,
                    const nsAString& aPosition,
                    int32_t aXPos,
                    int32_t aYPos,
                    bool aAttributesOverride);

  void GetAlignmentPosition(nsString& positionStr);

  int32_t AlignmentOffset();

private:
  ~PopupBoxObject();

protected:
  nsPopupSetFrame* GetPopupSetFrame();
};

} 
} 

#endif 
