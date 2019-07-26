





#ifndef DirectionalityUtils_h___
#define DirectionalityUtils_h___

class nsIContent;
class nsIDocument;
class nsINode;

namespace mozilla {
namespace dom {
class Element;
} 
} 

namespace mozilla {

namespace directionality {

enum Directionality {
  eDir_NotSet = 0,
  eDir_RTL    = 1,
  eDir_LTR    = 2
};








Directionality RecomputeDirectionality(mozilla::dom::Element* aElement,
                                       bool aNotify = true);








void SetDirectionalityOnDescendants(mozilla::dom::Element* aElement, 
                                    Directionality aDir,
                                    bool aNotify = true);

} 

} 

#endif 
