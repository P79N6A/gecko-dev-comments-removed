






#ifndef _nsARIAMap_H_
#define _nsARIAMap_H_

#include "ARIAStateMap.h"
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











const PRUint8 ATTR_BYPASSOBJ  = 0x0001;





const PRUint8 ATTR_VALTOKEN   = 0x0010;




struct nsAttributeCharacteristics
{
  nsIAtom** attributeName;
  const PRUint8 characteristics;
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

  


  const nsDependentAtomString ARIARoleString() const
    { return nsDependentAtomString(*roleAtom); }

  
  nsIAtom** roleAtom;

  
  mozilla::a11y::role role;
  
  
  bool roleRule;
  
  
  EValueRule valueRule;

  
  EActionRule actionRule;

  
  
  ELiveAttrRule liveAttRule;

  
  PRUint64 state;   

  
  
  
  
  
  mozilla::a11y::aria::EStateRule attributeMap1;
  mozilla::a11y::aria::EStateRule attributeMap2;
  mozilla::a11y::aria::EStateRule attributeMap3;
};










struct nsARIAMap
{
  




  static nsRoleMapEntry gEmptyRoleMap;

  


  static nsAttributeCharacteristics gWAIUnivAttrMap[];
  static PRUint32 gWAIUnivAttrMapLength;
};

namespace mozilla {
namespace a11y {
namespace aria {









nsRoleMapEntry* GetRoleMap(nsINode* aNode);





PRUint64 UniversalStatesFor(mozilla::dom::Element* aElement);

 



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
  PRUint32 mAttrIdx;
  PRUint32 mAttrCount;
};

} 
} 
} 

#endif
