





#ifndef DirectionalityUtils_h___
#define DirectionalityUtils_h___

#include "nscore.h"

class nsIContent;
class nsAString;
class nsAttrValue;
class nsTextNode;

namespace mozilla {
namespace dom {
class Element;
} 
} 

namespace mozilla {

enum Directionality {
  eDir_NotSet,
  eDir_RTL,
  eDir_LTR,
  eDir_Auto
};








Directionality RecomputeDirectionality(mozilla::dom::Element* aElement,
                                       bool aNotify = true);








void SetDirectionalityOnDescendants(mozilla::dom::Element* aElement,
                                    Directionality aDir,
                                    bool aNotify = true);







void WalkDescendantsResetAutoDirection(mozilla::dom::Element* aElement);









void WalkDescendantsSetDirAuto(mozilla::dom::Element* aElement,
                               bool aNotify = true);






void WalkDescendantsClearAncestorDirAuto(mozilla::dom::Element* aElement);







bool TextNodeWillChangeDirection(nsIContent* aTextNode, Directionality* aOldDir,
                                 uint32_t aOffset);





void TextNodeChangedDirection(nsIContent* aTextNode, Directionality aOldDir,
                              bool aNotify);





void SetDirectionFromNewTextNode(nsIContent* aTextNode);









void ResetDirectionSetByTextNode(nsTextNode* aTextNode, bool aNullParent);





void SetDirectionalityFromValue(mozilla::dom::Element* aElement,
                                const nsAString& aValue,
                                bool aNotify);









void OnSetDirAttr(mozilla::dom::Element* aElement,
                  const nsAttrValue* aNewValue,
                  bool hadValidDir,
                  bool hadDirAuto,
                  bool aNotify);






void SetDirOnBind(mozilla::dom::Element* aElement, nsIContent* aParent);






void ResetDir(mozilla::dom::Element* aElement);
} 

#endif 
