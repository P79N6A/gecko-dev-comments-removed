




#ifndef nsAccUtils_h_
#define nsAccUtils_h_

#include "nsIAccessible.h"
#include "nsIAccessibleRole.h"
#include "nsIAccessibleText.h"

#include "nsAccessibilityService.h"
#include "nsCoreUtils.h"

#include "mozilla/dom/Element.h"
#include "nsIDocShell.h"
#include "nsIPersistentProperties2.h"
#include "nsIPresShell.h"
#include "nsPoint.h"

struct nsRoleMapEntry;

namespace mozilla {
namespace a11y {

class Accessible;
class HyperTextAccessible;
class DocAccessible;

class nsAccUtils
{
public:
  






  static void GetAccAttr(nsIPersistentProperties *aAttributes,
                         nsIAtom *aAttrName,
                         nsAString& aAttrValue);

  






  static void SetAccAttr(nsIPersistentProperties *aAttributes,
                         nsIAtom *aAttrName,
                         const nsAString& aAttrValue);

  


  static void SetAccGroupAttrs(nsIPersistentProperties *aAttributes,
                               int32_t aLevel, int32_t aSetSize,
                               int32_t aPosInSet);

  


  static int32_t GetDefaultLevel(Accessible* aAcc);

  



  static int32_t GetARIAOrDefaultLevel(Accessible* aAccessible);

  


  static int32_t GetLevelForXULContainerItem(nsIContent *aContent);

  






  static void SetLiveContainerAttributes(nsIPersistentProperties *aAttributes,
                                         nsIContent *aStartContent,
                                         nsIContent *aTopContent);

  






  static bool HasDefinedARIAToken(nsIContent *aContent, nsIAtom *aAtom);

  


  static nsIAtom* GetARIAToken(mozilla::dom::Element* aElement, nsIAtom* aAttr);

  


  static DocAccessible* GetDocAccessibleFor(nsINode* aNode)
  {
    nsIPresShell *presShell = nsCoreUtils::GetPresShellFor(aNode);
    return GetAccService()->GetDocAccessible(presShell);
  }

  


  static DocAccessible* GetDocAccessibleFor(nsIDocShellTreeItem* aContainer)
  {
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aContainer));
    return GetAccService()->GetDocAccessible(docShell->GetPresShell());
  }

  







   static Accessible* GetAncestorWithRole(Accessible* aDescendant,
                                          uint32_t aRole);

  





  static Accessible* GetSelectableContainer(Accessible* aAccessible,
                                            uint64_t aState);

  



  static bool IsARIASelected(Accessible* aAccessible);

  






  static HyperTextAccessible*
    GetTextAccessibleFromSelection(nsISelection* aSelection);

  










  static nsIntPoint ConvertToScreenCoords(int32_t aX, int32_t aY,
                                          uint32_t aCoordinateType,
                                          Accessible* aAccessible);

  










  static void ConvertScreenCoordsTo(int32_t* aX, int32_t* aY,
                                    uint32_t aCoordinateType,
                                    Accessible* aAccessible);

  




  static nsIntPoint GetScreenCoordsForParent(Accessible* aAccessible);

  


  static uint32_t Role(nsIAccessible *aAcc)
  {
    uint32_t role = nsIAccessibleRole::ROLE_NOTHING;
    if (aAcc)
      aAcc->GetRole(&role);

    return role;
  }

  






  static uint8_t GetAttributeCharacteristics(nsIAtom* aAtom);

  








  static bool GetLiveAttrValue(uint32_t aRule, nsAString& aValue);

#ifdef DEBUG
  



  static bool IsTextInterfaceSupportCorrect(Accessible* aAccessible);
#endif

  


  static uint32_t TextLength(Accessible* aAccessible);

  


  static bool IsEmbeddedObject(nsIAccessible *aAcc)
  {
    uint32_t role = Role(aAcc);
    return role != nsIAccessibleRole::ROLE_TEXT_LEAF &&
           role != nsIAccessibleRole::ROLE_WHITESPACE &&
           role != nsIAccessibleRole::ROLE_STATICTEXT;
  }

  


  static inline uint64_t To64State(uint32_t aState1, uint32_t aState2)
  {
    return static_cast<uint64_t>(aState1) +
        (static_cast<uint64_t>(aState2) << 31);
  }

  


  static inline void To32States(uint64_t aState64,
                                uint32_t* aState1, uint32_t* aState2)
  {
    *aState1 = aState64 & 0x7fffffff;
    if (aState2)
      *aState2 = static_cast<uint32_t>(aState64 >> 31);
  }

  static uint32_t To32States(uint64_t aState, bool* aIsExtra)
  {
    uint32_t extraState = aState >> 31;
    *aIsExtra = !!extraState;
    return aState | extraState;
  }

  



  static bool MustPrune(Accessible* aAccessible);
};

} 
} 

#endif
