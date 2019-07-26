






#ifndef _nsARIAMap_H_
#define _nsARIAMap_H_

#include "ARIAStateMap.h"
#include "mozilla/a11y/AccTypes.h"
#include "mozilla/a11y/Role.h"

#include "nsIAtom.h"
#include "nsIContent.h"

class nsINode;







enum EValueRule
{
  


  eNoValue,

  



  eHasValueMinMax
};








enum EActionRule
{
  eNoAction,
  eActivateAction,
  eClickAction,
  ePressAction,
  eCheckUncheckAction,
  eExpandAction,
  eJumpAction,
  eOpenCloseAction,
  eSelectAction,
  eSortAction,
  eSwitchAction
};








enum ELiveAttrRule
{
  eNoLiveAttr,
  eOffLiveAttr,
  ePoliteLiveAttr
};








const bool kUseMapRole = true;




const bool kUseNativeRole = false;











const uint8_t ATTR_BYPASSOBJ = 0x1 << 0;
const uint8_t ATTR_BYPASSOBJ_IF_FALSE = 0x1 << 1;





const uint8_t ATTR_VALTOKEN = 0x1 << 2;





const uint8_t ATTR_GLOBAL = 0x1 << 3;




struct nsAttributeCharacteristics
{
  nsIAtom** attributeName;
  const uint8_t characteristics;
};









#define kNoReqStates 0

enum EDefaultStateRule
{
  
  eUseFirstState
};







struct nsRoleMapEntry
{
  


  bool Is(nsIAtom* aARIARole) const
    { return *roleAtom == aARIARole; }

  


  bool IsOfType(mozilla::a11y::AccGenericType aType) const
    { return accTypes & aType; }

  


  const nsDependentAtomString ARIARoleString() const
    { return nsDependentAtomString(*roleAtom); }

  
  nsIAtom** roleAtom;

  
  mozilla::a11y::role role;
  
  
  bool roleRule;
  
  
  EValueRule valueRule;

  
  EActionRule actionRule;

  
  
  ELiveAttrRule liveAttRule;

  
  uint32_t accTypes;

  
  uint64_t state;   

  
  
  
  
  
  mozilla::a11y::aria::EStateRule attributeMap1;
  mozilla::a11y::aria::EStateRule attributeMap2;
  mozilla::a11y::aria::EStateRule attributeMap3;
};










struct nsARIAMap
{
  




  static nsRoleMapEntry gEmptyRoleMap;

  


  static nsAttributeCharacteristics gWAIUnivAttrMap[];
  static uint32_t gWAIUnivAttrMapLength;
};

namespace mozilla {
namespace a11y {
namespace aria {









nsRoleMapEntry* GetRoleMap(nsINode* aNode);





uint64_t UniversalStatesFor(mozilla::dom::Element* aElement);

 



class AttrIterator
{
public:
  AttrIterator(nsIContent* aContent) : 
    mContent(aContent), mAttrIdx(0) 
  { 
    mAttrCount = mContent->GetAttrCount();
  }

  bool Next(nsAString& aAttrName, nsAString& aAttrValue);

private:
  AttrIterator() MOZ_DELETE;
  AttrIterator(const AttrIterator&) MOZ_DELETE;
  AttrIterator& operator= (const AttrIterator&) MOZ_DELETE;

  nsIContent* mContent;
  uint32_t mAttrIdx;
  uint32_t mAttrCount;
};

} 
} 
} 

#endif
