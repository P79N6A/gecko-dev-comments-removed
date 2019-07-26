





#ifndef DirectionalityUtils_h___
#define DirectionalityUtils_h___

#include "nscore.h"

class nsIContent;
class nsIDocument;
class nsINode;
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





void SetDirectionFromChangedTextNode(nsIContent* aTextNode, uint32_t aOffset,
                                     const PRUnichar* aBuffer, uint32_t aLength,
                                     bool aNotify);





void SetDirectionFromNewTextNode(nsIContent* aTextNode);





void ResetDirectionSetByTextNode(nsTextNode* aTextNode);





void SetDirectionalityFromValue(mozilla::dom::Element* aElement,
                                const nsAString& aValue,
                                bool aNotify);









void OnSetDirAttr(mozilla::dom::Element* aElement,
                  const nsAttrValue* aNewValue,
                  bool hadValidDir,
                  bool aNotify);






void SetDirOnBind(mozilla::dom::Element* aElement, nsIContent* aParent);






void ResetDir(mozilla::dom::Element* aElement);
} 

#endif 
