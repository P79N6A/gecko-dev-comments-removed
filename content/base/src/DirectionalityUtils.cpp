













































































































































































































#include "mozilla/dom/DirectionalityUtils.h"
#include "nsINode.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "mozilla/dom/Element.h"
#include "nsIDOMHTMLDocument.h"
#include "nsUnicodeProperties.h"
#include "nsTextFragment.h"
#include "nsAttrValue.h"
#include "nsContentUtils.h"
#include "nsTextNode.h"
#include "nsCheapSets.h"

namespace mozilla {

typedef mozilla::dom::Element Element;











static bool
DoesNotParticipateInAutoDirection(const Element* aElement)
{
  nsINodeInfo* nodeInfo = aElement->NodeInfo();
  return (!aElement->IsHTML() ||
          nodeInfo->Equals(nsGkAtoms::script) ||
          nodeInfo->Equals(nsGkAtoms::style) ||
          nodeInfo->Equals(nsGkAtoms::textarea) ||
          aElement->IsInAnonymousSubtree());
}

static inline bool
IsBdiWithoutDirAuto(const Element* aElement)
{
  
  
  
  return (aElement->IsHTML(nsGkAtoms::bdi) &&
          (!aElement->HasValidDir() || aElement->HasFixedDir()));
}






static bool
DoesNotAffectDirectionOfAncestors(const Element* aElement)
{
  return (DoesNotParticipateInAutoDirection(aElement) ||
          IsBdiWithoutDirAuto(aElement) ||
          aElement->HasFixedDir());
}




static Directionality
GetDirectionFromChar(uint32_t ch)
{
  switch(mozilla::unicode::GetBidiCat(ch)) {
    case eCharType_RightToLeft:
    case eCharType_RightToLeftArabic:
      return eDir_RTL;

    case eCharType_LeftToRight:
      return eDir_LTR;

    default:
      return eDir_NotSet;
  }
}

inline static bool NodeAffectsDirAutoAncestor(nsINode* aTextNode)
{
  Element* parent = aTextNode->GetParentElement();
  return (parent &&
          !DoesNotParticipateInAutoDirection(parent) &&
          parent->NodeOrAncestorHasDirAuto());
}










static Directionality
GetDirectionFromText(const PRUnichar* aText, const uint32_t aLength,
                     uint32_t* aFirstStrong = nullptr)
{
  const PRUnichar* start = aText;
  const PRUnichar* end = aText + aLength;

  while (start < end) {
    uint32_t current = start - aText;
    uint32_t ch = *start++;

    if (NS_IS_HIGH_SURROGATE(ch) &&
        start < end &&
        NS_IS_LOW_SURROGATE(*start)) {
      ch = SURROGATE_TO_UCS4(ch, *start++);
    }

    Directionality dir = GetDirectionFromChar(ch);
    if (dir != eDir_NotSet) {
      if (aFirstStrong) {
        *aFirstStrong = current;
      }
      return dir;
    }
  }

  if (aFirstStrong) {
    *aFirstStrong = UINT32_MAX;
  }
  return eDir_NotSet;
}

static Directionality
GetDirectionFromText(const char* aText, const uint32_t aLength,
                        uint32_t* aFirstStrong = nullptr)
{
  const char* start = aText;
  const char* end = aText + aLength;

  while (start < end) {
    uint32_t current = start - aText;
    unsigned char ch = (unsigned char)*start++;

    Directionality dir = GetDirectionFromChar(ch);
    if (dir != eDir_NotSet) {
      if (aFirstStrong) {
        *aFirstStrong = current;
      }
      return dir;
    }
  }

  if (aFirstStrong) {
    *aFirstStrong = UINT32_MAX;
  }
  return eDir_NotSet;
}

static Directionality
GetDirectionFromText(const nsTextFragment* aFrag,
                     uint32_t* aFirstStrong = nullptr)
{
  if (aFrag->Is2b()) {
    return GetDirectionFromText(aFrag->Get2b(), aFrag->GetLength(),
                                   aFirstStrong);
  }

  return GetDirectionFromText(aFrag->Get1b(), aFrag->GetLength(),
                                 aFirstStrong);
}







static nsINode*
WalkDescendantsSetDirectionFromText(Element* aElement, bool aNotify = true)
{
  MOZ_ASSERT(aElement, "aElement is null");
  if (DoesNotParticipateInAutoDirection(aElement)) {
    return nullptr;
  }

  nsIContent* child = aElement->GetFirstChild();
  while (child) {
    if (child->IsElement() &&
        DoesNotAffectDirectionOfAncestors(child->AsElement())) {
      child = child->GetNextNonChildNode(aElement);
      continue;
    }

    if (child->NodeType() == nsIDOMNode::TEXT_NODE) {
      Directionality textNodeDir = GetDirectionFromText(child->GetText());
      if (textNodeDir != eDir_NotSet) {
        
        
        aElement->SetDirectionality(textNodeDir, aNotify);
        return child;
      }
    }
    child = child->GetNextNode(aElement);
  }

  
  
  aElement->SetDirectionality(eDir_LTR, aNotify);
  return nullptr;
}

class nsTextNodeDirectionalityMap
{
  static void
  nsTextNodeDirectionalityMapDtor(void *aObject, nsIAtom* aPropertyName,
                                  void *aPropertyValue, void* aData)
  {
    nsINode* textNode = static_cast<nsINode * >(aObject);
    textNode->ClearHasTextNodeDirectionalityMap();

    nsTextNodeDirectionalityMap* map =
      reinterpret_cast<nsTextNodeDirectionalityMap * >(aPropertyValue);
    delete map;
  }

public:
  nsTextNodeDirectionalityMap(nsINode* aTextNode)
  {
    MOZ_ASSERT(aTextNode, "Null text node");
    MOZ_COUNT_CTOR(nsTextNodeDirectionalityMap);
    aTextNode->SetProperty(nsGkAtoms::textNodeDirectionalityMap, this,
                           nsTextNodeDirectionalityMapDtor);
    aTextNode->SetHasTextNodeDirectionalityMap();
  }

  ~nsTextNodeDirectionalityMap()
  {
    MOZ_COUNT_DTOR(nsTextNodeDirectionalityMap);
  }

  void AddEntry(nsINode* aTextNode, Element* aElement)
  {
    if (!mElements.Contains(aElement)) {
      mElements.Put(aElement);
      aElement->SetProperty(nsGkAtoms::dirAutoSetBy, aTextNode);
      aElement->SetHasDirAutoSet();
    }
  }

  void RemoveEntry(nsINode* aTextNode, Element* aElement)
  {
    if (mElements.Contains(aElement)) {
      mElements.Remove(aElement);

      aElement->ClearHasDirAutoSet();
      aElement->UnsetProperty(nsGkAtoms::dirAutoSetBy);
    }
  }

private:
  nsCheapSet<nsPtrHashKey<Element> > mElements;

  static nsTextNodeDirectionalityMap* GetDirectionalityMap(nsINode* aTextNode)
  {
    MOZ_ASSERT(aTextNode->NodeType() == nsIDOMNode::TEXT_NODE,
               "Must be a text node");
    nsTextNodeDirectionalityMap* map = nullptr;

    if (aTextNode->HasTextNodeDirectionalityMap()) {
      map = static_cast<nsTextNodeDirectionalityMap * >
        (aTextNode->GetProperty(nsGkAtoms::textNodeDirectionalityMap));
    }

    return map;
  }

  static PLDHashOperator SetNodeDirection(nsPtrHashKey<Element>* aEntry, void* aDir)
  {
    MOZ_ASSERT(aEntry->GetKey()->IsElement(), "Must be an Element");
    aEntry->GetKey()->SetDirectionality(*reinterpret_cast<Directionality*>(aDir),
                                        true);
    return PL_DHASH_NEXT;
  }

  static PLDHashOperator ResetNodeDirection(nsPtrHashKey<Element>* aEntry, void* aData)
  {
    MOZ_ASSERT(aEntry->GetKey()->IsElement(), "Must be an Element");
    
    
    Element* rootNode = aEntry->GetKey();
    nsINode* textNode = WalkDescendantsSetDirectionFromText(rootNode, true);
    if (textNode) {
      nsTextNodeDirectionalityMap::AddEntryToMap(textNode, rootNode);
    } else {
      rootNode->ClearHasDirAutoSet();
      rootNode->UnsetProperty(nsGkAtoms::dirAutoSetBy);
    }
    return PL_DHASH_REMOVE;
  }

public:
  void UpdateAutoDirection(Directionality aDir)
  {
    mElements.EnumerateEntries(SetNodeDirection, &aDir);
  }

  void ResetAutoDirection()
  {
    mElements.EnumerateEntries(ResetNodeDirection, nullptr);
  }

  static void RemoveElementFromMap(nsINode* aTextNode, Element* aElement)
  {
    if (aTextNode->HasTextNodeDirectionalityMap()) {
      GetDirectionalityMap(aTextNode)->RemoveEntry(aTextNode, aElement);
    }
  }

  static void AddEntryToMap(nsINode* aTextNode, Element* aElement)
  {
    nsTextNodeDirectionalityMap* map = GetDirectionalityMap(aTextNode);
    if (!map) {
      map = new nsTextNodeDirectionalityMap(aTextNode);
    }

    map->AddEntry(aTextNode, aElement);
  }

  static void UpdateTextNodeDirection(nsINode* aTextNode, Directionality aDir)
  {
    MOZ_ASSERT(aTextNode->HasTextNodeDirectionalityMap(),
               "Map missing in UpdateTextNodeDirection");
    GetDirectionalityMap(aTextNode)->UpdateAutoDirection(aDir);
  }

  static void ResetTextNodeDirection(nsINode* aTextNode)
  {
    MOZ_ASSERT(aTextNode->HasTextNodeDirectionalityMap(),
               "Map missing in ResetTextNodeDirection");
    GetDirectionalityMap(aTextNode)->ResetAutoDirection();
  }
};

Directionality
RecomputeDirectionality(Element* aElement, bool aNotify)
{
  MOZ_ASSERT(!aElement->HasDirAuto(),
             "RecomputeDirectionality called with dir=auto");

  Directionality dir = eDir_LTR;

  if (aElement->HasValidDir()) {
    dir = aElement->GetDirectionality();
  } else {
    Element* parent = aElement->GetParentElement();
    if (parent) {
      
      
      
      Directionality parentDir = parent->GetDirectionality();
      if (parentDir != eDir_NotSet) {
        dir = parentDir;
      }
    } else {
      
      
      Directionality documentDir =
        aElement->OwnerDoc()->GetDocumentDirectionality();
      if (documentDir != eDir_NotSet) {
        dir = documentDir;
      }
    }

    aElement->SetDirectionality(dir, aNotify);
  }
  return dir;
}

void
SetDirectionalityOnDescendants(Element* aElement, Directionality aDir,
                               bool aNotify)
{
  for (nsIContent* child = aElement->GetFirstChild(); child; ) {
    if (!child->IsElement()) {
      child = child->GetNextNode(aElement);
      continue;
    }

    Element* element = child->AsElement();
    if (element->HasValidDir() || element->HasDirAuto()) {
      child = child->GetNextNonChildNode(aElement);
      continue;
    }
    element->SetDirectionality(aDir, aNotify);
    child = child->GetNextNode(aElement);
  }
}






void
WalkAncestorsResetAutoDirection(Element* aElement, bool aNotify)
{
  nsINode* setByNode;
  Element* parent = aElement->GetParentElement();

  while (parent && parent->NodeOrAncestorHasDirAuto()) {
    if (parent->HasDirAutoSet()) {
      
      
      
      
      setByNode =
        static_cast<nsINode*>(parent->GetProperty(nsGkAtoms::dirAutoSetBy));
      if (setByNode) {
        nsTextNodeDirectionalityMap::RemoveElementFromMap(setByNode, parent);
      }
    }
    if (parent->HasDirAuto()) {
      setByNode = WalkDescendantsSetDirectionFromText(parent, aNotify);
      if (setByNode) {
        nsTextNodeDirectionalityMap::AddEntryToMap(setByNode, parent);
      }
      break;
    }
    parent = parent->GetParentElement();
  }
}

void
WalkDescendantsResetAutoDirection(Element* aElement)
{
  nsIContent* child = aElement->GetFirstChild();
  while (child) {
    if (child->HasDirAuto()) {
      child = child->GetNextNonChildNode(aElement);
      continue;
    }

    if (child->HasTextNodeDirectionalityMap()) {
      nsTextNodeDirectionalityMap::ResetTextNodeDirection(child);
    }
    child = child->GetNextNode(aElement);
  }
}

void
WalkDescendantsSetDirAuto(Element* aElement, bool aNotify)
{
  if (!DoesNotParticipateInAutoDirection(aElement) &&
      !IsBdiWithoutDirAuto(aElement)) {

    bool setAncestorDirAutoFlag =
#ifdef DEBUG
      true;
#else
      !aElement->AncestorHasDirAuto();
#endif

    if (setAncestorDirAutoFlag) {
      nsIContent* child = aElement->GetFirstChild();
      while (child) {
        if (child->IsElement() &&
            DoesNotAffectDirectionOfAncestors(child->AsElement())) {
          child = child->GetNextNonChildNode(aElement);
          continue;
        }

        MOZ_ASSERT(!aElement->AncestorHasDirAuto() ||
                   child->AncestorHasDirAuto(),
                   "AncestorHasDirAuto set on node but not its children");
        child->SetHasDirAuto();
        child = child->GetNextNode(aElement);
      }
    }
  }

  nsINode* textNode = WalkDescendantsSetDirectionFromText(aElement, aNotify);
  if (textNode) {
    nsTextNodeDirectionalityMap::AddEntryToMap(textNode, aElement);
  }
}

void
WalkDescendantsClearAncestorDirAuto(Element* aElement)
{
  nsIContent* child = aElement->GetFirstChild();
  while (child) {
    if (child->HasDirAuto()) {
      child = child->GetNextNonChildNode(aElement);
      continue;
    }

    child->ClearAncestorHasDirAuto();
    child = child->GetNextNode(aElement);
  }
}

void SetAncestorDirectionIfAuto(nsINode* aTextNode, Directionality aDir,
                                bool aNotify = true)
{
  MOZ_ASSERT(aTextNode->NodeType() == nsIDOMNode::TEXT_NODE,
             "Must be a text node");

  Element* parent = aTextNode->GetParentElement();
  while (parent && parent->NodeOrAncestorHasDirAuto()) {
    if (DoesNotParticipateInAutoDirection(parent) || parent->HasFixedDir()) {
      break;
    }

    if (parent->HasDirAuto()) {
      bool resetDirection = false;

      if (!parent->HasDirAutoSet()) {
        
        resetDirection = true;
      } else {
        
        
        
        
        
        nsINode* directionWasSetByTextNode =
          static_cast<nsINode*>(parent->GetProperty(nsGkAtoms::dirAutoSetBy));
        if (!directionWasSetByTextNode) {
          resetDirection = true;
        } else if (directionWasSetByTextNode != aTextNode) {
          nsIContent* child = aTextNode->GetNextNode(parent);
          while (child) {
            if (child->IsElement() &&
                DoesNotAffectDirectionOfAncestors(child->AsElement())) {
              child = child->GetNextNonChildNode(parent);
              continue;
            }

            if (child == directionWasSetByTextNode) {
              
              
              resetDirection = true;
              nsTextNodeDirectionalityMap::RemoveElementFromMap(
                directionWasSetByTextNode, parent
              );
              break;
            }

            child = child->GetNextNode(parent);
          }
        }
      }

      if (resetDirection) {
        parent->SetDirectionality(aDir, aNotify);
        nsTextNodeDirectionalityMap::AddEntryToMap(aTextNode, parent);
        SetDirectionalityOnDescendants(parent, aDir, aNotify);
      }

      
      
      
      return;
    }
    parent = parent->GetParentElement();
  }
}

void
SetDirectionFromChangedTextNode(nsIContent* aTextNode, uint32_t aOffset,
                                const PRUnichar* aBuffer, uint32_t aLength,
                                bool aNotify)
{
  if (!NodeAffectsDirAutoAncestor(aTextNode)) {
    return;
  }

  uint32_t firstStrong;
  Directionality oldDir = GetDirectionFromText(aTextNode->GetText(),
                                               &firstStrong);
  if (aOffset > firstStrong) {
    return;
  }

  Directionality newDir = GetDirectionFromText(aBuffer, aLength);
  if (newDir == eDir_NotSet) {
    if (oldDir != eDir_NotSet && aTextNode->HasTextNodeDirectionalityMap()) {
      
      
      
      
      nsTextNodeDirectionalityMap::ResetTextNodeDirection(aTextNode);
    }
  } else {
    
    
    
    
    
    
    if (aTextNode->HasTextNodeDirectionalityMap()) {
      nsTextNodeDirectionalityMap::UpdateTextNodeDirection(aTextNode, newDir);
    } else {
      SetAncestorDirectionIfAuto(aTextNode, newDir, aNotify);
    }
  }
}

void
SetDirectionFromNewTextNode(nsIContent* aTextNode)
{
  if (!NodeAffectsDirAutoAncestor(aTextNode)) {
    return;
  }

  Element* parent = aTextNode->GetParentElement();
  if (parent && parent->NodeOrAncestorHasDirAuto()) {
    aTextNode->SetAncestorHasDirAuto();
  }

  Directionality dir = GetDirectionFromText(aTextNode->GetText());
  if (dir != eDir_NotSet) {
    SetAncestorDirectionIfAuto(aTextNode, dir);
  }
}

void
ResetDirectionSetByTextNode(nsTextNode* aTextNode)
{
  if (!NodeAffectsDirAutoAncestor(aTextNode)) {
    return;
  }

  Directionality dir = GetDirectionFromText(aTextNode->GetText());
  if (dir != eDir_NotSet && aTextNode->HasTextNodeDirectionalityMap()) {
    nsTextNodeDirectionalityMap::ResetTextNodeDirection(aTextNode);
  }
}

void
SetDirectionalityFromValue(Element* aElement, const nsAString& value,
                           bool aNotify)
{
  Directionality dir = GetDirectionFromText(PromiseFlatString(value).get(),
                                            value.Length());
  if (dir == eDir_NotSet) {
    dir = eDir_LTR;
  }

  aElement->SetDirectionality(dir, aNotify);
}

void
OnSetDirAttr(Element* aElement, const nsAttrValue* aNewValue,
             bool hadValidDir, bool aNotify)
{
  if (aElement->IsHTML(nsGkAtoms::input)) {
    return;
  }

  if (aElement->AncestorHasDirAuto()) {
    if (!hadValidDir) {
      
      
      
      
      
      WalkDescendantsResetAutoDirection(aElement);
    } else if (!aElement->HasValidDir()) {
      
      
      
      
      WalkAncestorsResetAutoDirection(aElement, aNotify);
    }
  }

  if (aElement->HasDirAuto()) {
    WalkDescendantsSetDirAuto(aElement, aNotify);
  } else {
    SetDirectionalityOnDescendants(aElement,
                                   RecomputeDirectionality(aElement, aNotify),
                                   aNotify);
  }
}

void
SetDirOnBind(mozilla::dom::Element* aElement, nsIContent* aParent)
{
  
  
  if (!DoesNotParticipateInAutoDirection(aElement) &&
      !aElement->IsHTML(nsGkAtoms::bdi) &&
      aParent && aParent->NodeOrAncestorHasDirAuto()) {
    aElement->SetAncestorHasDirAuto();

    nsIContent* child = aElement->GetFirstChild();
    if (child) {
      
      
      
      
      do {
        if (child->IsElement() &&
            DoesNotAffectDirectionOfAncestors(child->AsElement())) {
          child = child->GetNextNonChildNode(aElement);
          continue;
        }

        child->SetAncestorHasDirAuto();
        child = child->GetNextNode(aElement);
      } while (child);

      
      WalkAncestorsResetAutoDirection(aElement, true);
    }
  }

  if (!aElement->HasDirAuto()) {
    
    
    RecomputeDirectionality(aElement, false);
  }
}

void ResetDir(mozilla::dom::Element* aElement)
{
  if (aElement->HasDirAutoSet()) {
    nsINode* setByNode =
      static_cast<nsINode*>(aElement->GetProperty(nsGkAtoms::dirAutoSetBy));
    nsTextNodeDirectionalityMap::RemoveElementFromMap(setByNode, aElement);
  }

  if (!aElement->HasDirAuto()) {
    RecomputeDirectionality(aElement, false);
  }
}

} 

