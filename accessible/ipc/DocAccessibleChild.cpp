





#include "DocAccessibleChild.h"

#include "Accessible-inl.h"
#include "ProxyAccessible.h"
#include "Relation.h"
#include "HyperTextAccessible-inl.h"
#include "ImageAccessible.h"
#include "TableAccessible.h"
#include "TableCellAccessible.h"
#include "nsIPersistentProperties2.h"
#include "nsISimpleEnumerator.h"

namespace mozilla {
namespace a11y {

static uint32_t
InterfacesFor(Accessible* aAcc)
{
  uint32_t interfaces = 0;
  if (aAcc->IsHyperText() && aAcc->AsHyperText()->IsTextRole())
    interfaces |= Interfaces::HYPERTEXT;

  return interfaces;
}

static void
SerializeTree(Accessible* aRoot, nsTArray<AccessibleData>& aTree)
{
  uint64_t id = reinterpret_cast<uint64_t>(aRoot->UniqueID());
  uint32_t role = aRoot->Role();
  uint32_t childCount = aRoot->ChildCount();
  uint32_t interfaces = InterfacesFor(aRoot);

  
  
  
  if (childCount == 1 && aRoot->GetChildAt(0)->IsDoc())
    childCount = 0;

  aTree.AppendElement(AccessibleData(id, role, childCount, interfaces));
  for (uint32_t i = 0; i < childCount; i++)
    SerializeTree(aRoot->GetChildAt(i), aTree);
}

Accessible*
DocAccessibleChild::IdToAccessible(const uint64_t& aID) const
{
  return mDoc->GetAccessibleByUniqueID(reinterpret_cast<void*>(aID));
}

Accessible*
DocAccessibleChild::IdToAccessibleLink(const uint64_t& aID) const
{
  Accessible* acc = IdToAccessible(aID);
  return acc && acc->IsLink() ? acc : nullptr;
}

HyperTextAccessible*
DocAccessibleChild::IdToHyperTextAccessible(const uint64_t& aID) const
{
  Accessible* acc = IdToAccessible(aID);
  return acc && acc->IsHyperText() ? acc->AsHyperText() : nullptr;
}

ImageAccessible*
DocAccessibleChild::IdToImageAccessible(const uint64_t& aID) const
{
  Accessible* acc = IdToAccessible(aID);
  return (acc && acc->IsImage()) ? acc->AsImage() : nullptr;
}

TableCellAccessible*
DocAccessibleChild::IdToTableCellAccessible(const uint64_t& aID) const
{
  Accessible* acc = IdToAccessible(aID);
  return (acc && acc->IsTableCell()) ? acc->AsTableCell() : nullptr;
}

TableAccessible*
DocAccessibleChild::IdToTableAccessible(const uint64_t& aID) const
{
  Accessible* acc = IdToAccessible(aID);
  return (acc && acc->IsTable()) ? acc->AsTable() : nullptr;
}

void
DocAccessibleChild::ShowEvent(AccShowEvent* aShowEvent)
{
  Accessible* parent = aShowEvent->Parent();
  uint64_t parentID = parent->IsDoc() ? 0 : reinterpret_cast<uint64_t>(parent->UniqueID());
  uint32_t idxInParent = aShowEvent->GetAccessible()->IndexInParent();
  nsTArray<AccessibleData> shownTree;
  ShowEventData data(parentID, idxInParent, shownTree);
  SerializeTree(aShowEvent->GetAccessible(), data.NewTree());
  SendShowEvent(data);
}

bool
DocAccessibleChild::RecvState(const uint64_t& aID, uint64_t* aState)
{
  Accessible* acc = IdToAccessible(aID);
  if (!acc) {
    *aState = states::DEFUNCT;
    return true;
  }

  *aState = acc->State();

  return true;
}

bool
DocAccessibleChild::RecvName(const uint64_t& aID, nsString* aName)
{
  Accessible* acc = IdToAccessible(aID);
  if (!acc)
    return true;

  acc->Name(*aName);
  return true;
}

bool
DocAccessibleChild::RecvValue(const uint64_t& aID, nsString* aValue)
{
  Accessible* acc = IdToAccessible(aID);
  if (!acc) {
    return true;
  }

  acc->Value(*aValue);
  return true;
}

bool
DocAccessibleChild::RecvDescription(const uint64_t& aID, nsString* aDesc)
{
  Accessible* acc = IdToAccessible(aID);
  if (!acc)
    return true;

  acc->Description(*aDesc);
  return true;
}

bool
DocAccessibleChild::RecvAttributes(const uint64_t& aID, nsTArray<Attribute>* aAttributes)
{
  Accessible* acc = IdToAccessible(aID);
  if (!acc)
    return true;

  nsCOMPtr<nsIPersistentProperties> props = acc->Attributes();
  return PersistentPropertiesToArray(props, aAttributes);
}

bool
DocAccessibleChild::PersistentPropertiesToArray(nsIPersistentProperties* aProps,
                                                nsTArray<Attribute>* aAttributes)
{
  if (!aProps) {
    return true;
  }
  nsCOMPtr<nsISimpleEnumerator> propEnum;
  nsresult rv = aProps->Enumerate(getter_AddRefs(propEnum));
  NS_ENSURE_SUCCESS(rv, false);

  bool hasMore;
  while (NS_SUCCEEDED(propEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> sup;
    rv = propEnum->GetNext(getter_AddRefs(sup));
    NS_ENSURE_SUCCESS(rv, false);

    nsCOMPtr<nsIPropertyElement> propElem(do_QueryInterface(sup));
    NS_ENSURE_TRUE(propElem, false);

    nsAutoCString name;
    rv = propElem->GetKey(name);
    NS_ENSURE_SUCCESS(rv, false);

    nsAutoString value;
    rv = propElem->GetValue(value);
    NS_ENSURE_SUCCESS(rv, false);

    aAttributes->AppendElement(Attribute(name, value));
    }

  return true;
}

bool
DocAccessibleChild::RecvRelationByType(const uint64_t& aID,
                                       const uint32_t& aType,
                                       nsTArray<uint64_t>* aTargets)
{
  Accessible* acc = mDoc->GetAccessibleByUniqueID((void*)aID);
  if (!acc)
    return false;

  auto type = static_cast<RelationType>(aType);
  Relation rel = acc->RelationByType(type);
  while (Accessible* target = rel.Next())
    aTargets->AppendElement(reinterpret_cast<uintptr_t>(target));

  return true;
}

static void
AddRelation(Accessible* aAcc, RelationType aType,
            nsTArray<RelationTargets>* aTargets)
{
  Relation rel = aAcc->RelationByType(aType);
  nsTArray<uint64_t> targets;
  while (Accessible* target = rel.Next())
    targets.AppendElement(reinterpret_cast<uintptr_t>(target));

  if (!targets.IsEmpty()) {
    RelationTargets* newRelation =
      aTargets->AppendElement(RelationTargets(static_cast<uint32_t>(aType),
                                              nsTArray<uint64_t>()));
    newRelation->Targets().SwapElements(targets);
  }
}

bool
DocAccessibleChild::RecvRelations(const uint64_t& aID,
                                  nsTArray<RelationTargets>* aRelations)
{
  Accessible* acc = mDoc->GetAccessibleByUniqueID((void*)aID);
  if (!aID)
    return false;

#define RELATIONTYPE(gecko, s, a, m, i) AddRelation(acc, RelationType::gecko, aRelations);

#include "RelationTypeMap.h"
#undef RELATIONTYPE

  return true;
}

bool
DocAccessibleChild::RecvCaretOffset(const uint64_t& aID, int32_t* aOffset)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  *aOffset = acc && acc->IsTextRole() ? acc->CaretOffset() : 0;
  return true;
}

bool
DocAccessibleChild::RecvSetCaretOffset(const uint64_t& aID,
                                       const int32_t& aOffset,
                                       bool* aRetVal)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  *aRetVal = false;
  if (acc && acc->IsTextRole() && acc->IsValidOffset(aOffset)) {
    *aRetVal = true;
    acc->SetCaretOffset(aOffset);
  }
  return true;
}

bool
DocAccessibleChild::RecvCharacterCount(const uint64_t& aID, int32_t* aCount)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  *aCount = acc ? acc->CharacterCount() : 0;
  return true;
}

bool
DocAccessibleChild::RecvSelectionCount(const uint64_t& aID, int32_t* aCount)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  *aCount = acc ? acc->SelectionCount() : 0;
  return true;
}

bool
DocAccessibleChild::RecvTextSubstring(const uint64_t& aID,
                                      const int32_t& aStartOffset,
                                      const int32_t& aEndOffset,
                                      nsString* aText)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (!acc) {
    return true;
  }

  acc->TextSubstring(aStartOffset, aEndOffset, *aText);
  return true;
}

bool
DocAccessibleChild::RecvGetTextAfterOffset(const uint64_t& aID,
                                           const int32_t& aOffset,
                                           const int32_t& aBoundaryType,
                                           nsString* aText,
                                           int32_t* aStartOffset,
                                           int32_t* aEndOffset)
{
  *aStartOffset = 0;
  *aEndOffset = 0;
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc) {
    acc->TextAfterOffset(aOffset, aBoundaryType,
                         aStartOffset, aEndOffset, *aText);
  }
  return true;
}

bool
DocAccessibleChild::RecvGetTextAtOffset(const uint64_t& aID,
                                        const int32_t& aOffset,
                                        const int32_t& aBoundaryType,
                                        nsString* aText,
                                        int32_t* aStartOffset,
                                        int32_t* aEndOffset)
{
  *aStartOffset = 0;
  *aEndOffset = 0;
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc) {
    acc->TextAtOffset(aOffset, aBoundaryType,
                      aStartOffset, aEndOffset, *aText);
  }
  return true;
}

bool
DocAccessibleChild::RecvGetTextBeforeOffset(const uint64_t& aID,
                                            const int32_t& aOffset,
                                            const int32_t& aBoundaryType,
                                            nsString* aText,
                                            int32_t* aStartOffset,
                                            int32_t* aEndOffset)
{
  *aStartOffset = 0;
  *aEndOffset = 0;
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc) {
    acc->TextBeforeOffset(aOffset, aBoundaryType,
                          aStartOffset, aEndOffset, *aText);
  }
  return true;
}

bool
DocAccessibleChild::RecvCharAt(const uint64_t& aID,
                               const int32_t& aOffset,
                               uint16_t* aChar)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  *aChar = acc && acc->IsTextRole() ?
    static_cast<uint16_t>(acc->CharAt(aOffset)) : 0;
  return true;
}

bool
DocAccessibleChild::RecvTextAttributes(const uint64_t& aID,
                                       const bool& aIncludeDefAttrs,
                                       const int32_t& aOffset,
                                       nsTArray<Attribute>* aAttributes,
                                       int32_t* aStartOffset,
                                       int32_t* aEndOffset)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (!acc || !acc->IsTextRole()) {
    return true;
  }

  nsCOMPtr<nsIPersistentProperties> props =
    acc->TextAttributes(aIncludeDefAttrs, aOffset, aStartOffset, aEndOffset);
  return PersistentPropertiesToArray(props, aAttributes);
}

bool
DocAccessibleChild::RecvDefaultTextAttributes(const uint64_t& aID,
                                              nsTArray<Attribute> *aAttributes)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (!acc || !acc->IsTextRole()) {
    return true;
  }

  nsCOMPtr<nsIPersistentProperties> props = acc->DefaultTextAttributes();
  return PersistentPropertiesToArray(props, aAttributes);
}

bool
DocAccessibleChild::RecvTextBounds(const uint64_t& aID,
                                   const int32_t& aStartOffset,
                                   const int32_t& aEndOffset,
                                   const uint32_t& aCoordType,
                                   nsIntRect* aRetVal)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc && acc->IsTextRole()) {
    *aRetVal = acc->TextBounds(aStartOffset, aEndOffset, aCoordType);
  }

  return true;
}

bool
DocAccessibleChild::RecvCharBounds(const uint64_t& aID,
                                   const int32_t& aOffset,
                                   const uint32_t& aCoordType,
                                   nsIntRect* aRetVal)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc && acc->IsTextRole()) {
    *aRetVal = acc->CharBounds(aOffset, aCoordType);
  }

  return true;
}

bool
DocAccessibleChild::RecvOffsetAtPoint(const uint64_t& aID,
                                      const int32_t& aX,
                                      const int32_t& aY,
                                      const uint32_t& aCoordType,
                                      int32_t* aRetVal)
{
  *aRetVal = -1;
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc && acc->IsTextRole()) {
    *aRetVal = acc->OffsetAtPoint(aX, aY, aCoordType);
  }
  return true;
}

bool
DocAccessibleChild::RecvSelectionBoundsAt(const uint64_t& aID,
                                          const int32_t& aSelectionNum,
                                          bool* aSucceeded,
                                          nsString* aData,
                                          int32_t* aStartOffset,
                                          int32_t* aEndOffset)
{
  *aSucceeded = false;
  *aStartOffset = 0;
  *aEndOffset = 0;
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc && acc->IsTextRole()) {
    *aSucceeded =
      acc->SelectionBoundsAt(aSelectionNum, aStartOffset, aEndOffset);
    if (*aSucceeded) {
      acc->TextSubstring(*aStartOffset, *aEndOffset, *aData);
    }
  }

  return true;
}

bool
DocAccessibleChild::RecvSetSelectionBoundsAt(const uint64_t& aID,
                                             const int32_t& aSelectionNum,
                                             const int32_t& aStartOffset,
                                             const int32_t& aEndOffset,
                                             bool* aSucceeded)
{
  *aSucceeded = false;
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc && acc->IsTextRole()) {
    *aSucceeded =
      acc->SetSelectionBoundsAt(aSelectionNum, aStartOffset, aEndOffset);
  }

  return true;
}

bool
DocAccessibleChild::RecvAddToSelection(const uint64_t& aID,
                                       const int32_t& aStartOffset,
                                       const int32_t& aEndOffset,
                                       bool* aSucceeded)
{
  *aSucceeded = false;
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc && acc->IsTextRole()) {
    *aSucceeded = acc->AddToSelection(aStartOffset, aEndOffset);
  }

  return true;
}

bool
DocAccessibleChild::RecvRemoveFromSelection(const uint64_t& aID,
                                            const int32_t& aSelectionNum,
                                            bool* aSucceeded)
{
  *aSucceeded = false;
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc && acc->IsTextRole()) {
    *aSucceeded = acc->RemoveFromSelection(aSelectionNum);
  }

  return true;
}

bool
DocAccessibleChild::RecvScrollSubstringTo(const uint64_t& aID,
                                          const int32_t& aStartOffset,
                                          const int32_t& aEndOffset,
                                          const uint32_t& aScrollType)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc) {
    acc->ScrollSubstringTo(aStartOffset, aEndOffset, aScrollType);
  }

  return true;
}

bool
DocAccessibleChild::RecvScrollSubstringToPoint(const uint64_t& aID,
                                               const int32_t& aStartOffset,
                                               const int32_t& aEndOffset,
                                               const uint32_t& aCoordinateType,
                                               const int32_t& aX,
                                               const int32_t& aY)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc) {
    acc->ScrollSubstringToPoint(aStartOffset, aEndOffset, aCoordinateType,
                                aX, aY);
  }

  return true;
}


bool
DocAccessibleChild::RecvReplaceText(const uint64_t& aID,
                                    const nsString& aText)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc && acc->IsTextRole()) {
    acc->ReplaceText(aText);
  }

  return true;
}

bool
DocAccessibleChild::RecvInsertText(const uint64_t& aID,
                                   const nsString& aText,
                                   const int32_t& aPosition)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc && acc->IsTextRole()) {
    acc->InsertText(aText, aPosition);
  }

  return true;
}

bool
DocAccessibleChild::RecvCopyText(const uint64_t& aID,
                                 const int32_t& aStartPos,
                                 const int32_t& aEndPos)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc && acc->IsTextRole()) {
    acc->CopyText(aStartPos, aEndPos);
  }

  return true;
}

bool
DocAccessibleChild::RecvCutText(const uint64_t& aID,
                                const int32_t& aStartPos,
                                const int32_t& aEndPos)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc && acc->IsTextRole()) {
    acc->CutText(aStartPos, aEndPos);
  }

  return true;
}

bool
DocAccessibleChild::RecvDeleteText(const uint64_t& aID,
                                   const int32_t& aStartPos,
                                   const int32_t& aEndPos)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc && acc->IsTextRole()) {
    acc->DeleteText(aStartPos, aEndPos);
  }

  return true;
}

bool
DocAccessibleChild::RecvPasteText(const uint64_t& aID,
                                  const int32_t& aPosition)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc && acc->IsTextRole()) {
    acc->PasteText(aPosition);
  }

  return true;
}

bool
DocAccessibleChild::RecvImagePosition(const uint64_t& aID,
                                      const uint32_t& aCoordType,
                                      nsIntPoint* aRetVal)
{
  ImageAccessible* acc = IdToImageAccessible(aID);
  if (acc) {
    *aRetVal = acc->Position(aCoordType);
  }

  return true;
}

bool
DocAccessibleChild::RecvImageSize(const uint64_t& aID,
                                  nsIntSize* aRetVal)
{

  ImageAccessible* acc = IdToImageAccessible(aID);
  if (acc) {
    *aRetVal = acc->Size();
  }

  return true;
}

bool
DocAccessibleChild::RecvStartOffset(const uint64_t& aID,
                                    uint32_t* aRetVal,
                                    bool* aOk)
{
  Accessible* acc = IdToAccessibleLink(aID);
  if (acc) {
    *aRetVal = acc->StartOffset();
    *aOk = true;
  } else {
    *aRetVal = 0;
    *aOk = false;
  }

  return true;
}

bool
DocAccessibleChild::RecvEndOffset(const uint64_t& aID,
                                  uint32_t* aRetVal,
                                  bool* aOk)
{
  Accessible* acc = IdToAccessibleLink(aID);
  if (acc) {
    *aRetVal = acc->EndOffset();
    *aOk = true;
  } else {
    *aRetVal = 0;
    *aOk = false;
  }

  return true;
}

bool
DocAccessibleChild::RecvIsLinkValid(const uint64_t& aID,
                                    bool* aRetVal)
{
  Accessible* acc = IdToAccessibleLink(aID);
  if (acc) {
    *aRetVal = acc->IsLinkValid();
  } else {
    *aRetVal = false;
  }

  return true;
}

bool
DocAccessibleChild::RecvAnchorCount(const uint64_t& aID,
                                    uint32_t* aRetVal,
                                    bool* aOk)
{
  Accessible* acc = IdToAccessibleLink(aID);
  if (acc) {
    *aRetVal = acc->AnchorCount();
    *aOk = true;
  } else {
    *aRetVal = 0;
    *aOk = false;
  }

  return true;
}

bool
DocAccessibleChild::RecvAnchorURIAt(const uint64_t& aID,
                                    const uint32_t& aIndex,
                                    nsCString* aURI,
                                    bool* aOk)
{
  Accessible* acc = IdToAccessibleLink(aID);
  *aOk = false;
  if (acc) {
    nsCOMPtr<nsIURI> uri = acc->AnchorURIAt(aIndex);
    if (uri) {
      uri->GetSpec(*aURI);
      *aOk = true;
    }
  }

  return true;
}

bool
DocAccessibleChild::RecvAnchorAt(const uint64_t& aID,
                                 const uint32_t& aIndex,
                                 uint64_t* aIDOfAnchor,
                                 bool* aOk)
{
  *aIDOfAnchor = 0;
  *aOk = false;
  Accessible* acc = IdToAccessibleLink(aID);
  if (acc) {
    Accessible* anchor = acc->AnchorAt(aIndex);
    if (anchor) {
      *aIDOfAnchor = reinterpret_cast<uint64_t>(anchor->UniqueID());
      *aOk = true;
    }
  }

  return true;
}

bool
DocAccessibleChild::RecvLinkCount(const uint64_t& aID,
                                  uint32_t* aCount)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  *aCount = acc ? acc->LinkCount() : 0;
  return true;
}

bool
DocAccessibleChild::RecvLinkAt(const uint64_t& aID,
                               const uint32_t& aIndex,
                               uint64_t* aIDOfLink,
                               bool* aOk)
{
  *aIDOfLink = 0;
  *aOk = false;
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  if (acc) {
    Accessible* link = acc->LinkAt(aIndex);
    if (link) {
      *aIDOfLink = reinterpret_cast<uint64_t>(link->UniqueID());
      *aOk = true;
    }
  }

  return true;
}

bool
DocAccessibleChild::RecvLinkIndexOf(const uint64_t& aID,
                                    const uint64_t& aLinkID,
                                    int32_t* aIndex)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  Accessible* link = IdToAccessible(aLinkID);
  *aIndex = -1;
  if (acc && link) {
    *aIndex = acc->LinkIndexOf(link);
  }

  return true;
}

bool
DocAccessibleChild::RecvLinkIndexAtOffset(const uint64_t& aID,
                                          const uint32_t& aOffset,
                                          int32_t* aIndex)
{
  HyperTextAccessible* acc = IdToHyperTextAccessible(aID);
  *aIndex = acc ? acc->LinkIndexAtOffset(aOffset) : -1;
  return true;
}

bool
DocAccessibleChild::RecvTableOfACell(const uint64_t& aID,
                                     uint64_t* aTableID,
                                     bool* aOk)
{
  *aTableID = 0;
  *aOk = false;
  TableCellAccessible* acc = IdToTableCellAccessible(aID);
  if (acc) {
    TableAccessible* table = acc->Table();
    if (table) {
      *aTableID = reinterpret_cast<uint64_t>(table->AsAccessible()->UniqueID());
      *aOk = true;
    }
  }

  return true;
}

bool
DocAccessibleChild::RecvColIdx(const uint64_t& aID,
                               uint32_t* aIndex)
{
  *aIndex = 0;
  TableCellAccessible* acc = IdToTableCellAccessible(aID);
  if (acc) {
    *aIndex = acc->ColIdx();
  }

  return true;
}

bool
DocAccessibleChild::RecvRowIdx(const uint64_t& aID,
                               uint32_t* aIndex)
{
  *aIndex = 0;
  TableCellAccessible* acc = IdToTableCellAccessible(aID);
  if (acc) {
    *aIndex = acc->RowIdx();
  }

  return true;
}

bool
DocAccessibleChild::RecvColExtent(const uint64_t& aID,
                                  uint32_t* aExtent)
{
  *aExtent = 0;
  TableCellAccessible* acc = IdToTableCellAccessible(aID);
  if (acc) {
    *aExtent = acc->ColExtent();
  }

  return true;
}

bool
DocAccessibleChild::RecvRowExtent(const uint64_t& aID,
                                  uint32_t* aExtent)
{
  *aExtent = 0;
  TableCellAccessible* acc = IdToTableCellAccessible(aID);
  if (acc) {
    *aExtent = acc->RowExtent();
  }

  return true;
}

bool
DocAccessibleChild::RecvColHeaderCells(const uint64_t& aID,
                                       nsTArray<uint64_t>* aCells)
{
  TableCellAccessible* acc = IdToTableCellAccessible(aID);
  if (acc) {
    nsAutoTArray<Accessible*, 10> headerCells;
    acc->ColHeaderCells(&headerCells);
    aCells->SetCapacity(headerCells.Length());
    for (uint32_t i = 0; i < headerCells.Length(); ++i) {
      aCells->AppendElement(
        reinterpret_cast<uint64_t>(headerCells[i]->UniqueID()));
    }
  }

  return true;
}

bool
DocAccessibleChild::RecvRowHeaderCells(const uint64_t& aID,
                                       nsTArray<uint64_t>* aCells)
{
  TableCellAccessible* acc = IdToTableCellAccessible(aID);
  if (acc) {
    nsAutoTArray<Accessible*, 10> headerCells;
    acc->RowHeaderCells(&headerCells);
    aCells->SetCapacity(headerCells.Length());
    for (uint32_t i = 0; i < headerCells.Length(); ++i) {
      aCells->AppendElement(
        reinterpret_cast<uint64_t>(headerCells[i]->UniqueID()));
    }
  }

  return true;
}

bool
DocAccessibleChild::RecvIsCellSelected(const uint64_t& aID,
                                       bool* aSelected)
{
  TableCellAccessible* acc = IdToTableCellAccessible(aID);
  *aSelected = acc && acc->Selected();
  return true;
}

bool
DocAccessibleChild::RecvTableCaption(const uint64_t& aID,
                                     uint64_t* aCaptionID,
                                     bool* aOk)
{
  *aCaptionID = 0;
  *aOk = false;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    Accessible* caption = acc->Caption();
    if (caption) {
      *aCaptionID = reinterpret_cast<uint64_t>(caption->UniqueID());
      *aOk = true;
    }
  }

  return true;
}

bool
DocAccessibleChild::RecvTableSummary(const uint64_t& aID,
                                     nsString* aSummary)
{
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    acc->Summary(*aSummary);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableColumnCount(const uint64_t& aID,
                                         uint32_t* aColCount)
{
  *aColCount = 0;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aColCount = acc->ColCount();
  }

  return true;
}

bool
DocAccessibleChild::RecvTableRowCount(const uint64_t& aID,
                                      uint32_t* aRowCount)
{
  *aRowCount = 0;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aRowCount = acc->RowCount();
  }

  return true;
}

bool
DocAccessibleChild::RecvTableCellAt(const uint64_t& aID,
                                    const uint32_t& aRow,
                                    const uint32_t& aCol,
                                    uint64_t* aCellID,
                                    bool* aOk)
{
  *aCellID = 0;
  *aOk = false;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    Accessible* cell = acc->CellAt(aRow, aCol);
    if (cell) {
      *aCellID = reinterpret_cast<uint64_t>(cell->UniqueID());
      *aOk = true;
    }
  }

  return true;
}

bool
DocAccessibleChild::RecvTableCellIndexAt(const uint64_t& aID,
                                         const uint32_t& aRow,
                                         const uint32_t& aCol,
                                         int32_t* aIndex)
{
  *aIndex = -1;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aIndex = acc->CellIndexAt(aRow, aCol);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableColumnIndexAt(const uint64_t& aID,
                                           const uint32_t& aCellIndex,
                                           int32_t* aCol)
{
  *aCol = -1;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aCol = acc->ColIndexAt(aCellIndex);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableRowIndexAt(const uint64_t& aID,
                                        const uint32_t& aCellIndex,
                                        int32_t* aRow)
{
  *aRow = -1;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aRow = acc->RowIndexAt(aCellIndex);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableRowAndColumnIndicesAt(const uint64_t& aID,
                                                  const uint32_t& aCellIndex,
                                                  int32_t* aRow,
                                                  int32_t* aCol)
{
  *aRow = -1;
  *aCol = -1;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    acc->RowAndColIndicesAt(aCellIndex, aRow, aCol);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableColumnExtentAt(const uint64_t& aID,
                                            const uint32_t& aRow,
                                            const uint32_t& aCol,
                                            uint32_t* aExtent)
{
  *aExtent = 0;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aExtent = acc->ColExtentAt(aRow, aCol);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableRowExtentAt(const uint64_t& aID,
                                         const uint32_t& aRow,
                                         const uint32_t& aCol,
                                         uint32_t* aExtent)
{
  *aExtent = 0;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aExtent = acc->RowExtentAt(aRow, aCol);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableColumnDescription(const uint64_t& aID,
                                               const uint32_t& aCol,
                                               nsString* aDescription)
{
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    acc->ColDescription(aCol, *aDescription);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableRowDescription(const uint64_t& aID,
                                            const uint32_t& aRow,
                                            nsString* aDescription)
{
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    acc->RowDescription(aRow, *aDescription);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableColumnSelected(const uint64_t& aID,
                                            const uint32_t& aCol,
                                            bool* aSelected)
{
  *aSelected = false;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aSelected = acc->IsColSelected(aCol);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableRowSelected(const uint64_t& aID,
                                         const uint32_t& aRow,
                                         bool* aSelected)
{
  *aSelected = false;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aSelected = acc->IsRowSelected(aRow);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableCellSelected(const uint64_t& aID,
                                          const uint32_t& aRow,
                                          const uint32_t& aCol,
                                          bool* aSelected)
{
  *aSelected = false;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aSelected = acc->IsCellSelected(aRow, aCol);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableSelectedCellCount(const uint64_t& aID,
                                               uint32_t* aSelectedCells)
{
  *aSelectedCells = 0;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aSelectedCells = acc->SelectedCellCount();
  }

  return true;
}

bool
DocAccessibleChild::RecvTableSelectedColumnCount(const uint64_t& aID,
                                                 uint32_t* aSelectedColumns)
{
  *aSelectedColumns = 0;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aSelectedColumns = acc->SelectedColCount();
  }

  return true;
}

bool
DocAccessibleChild::RecvTableSelectedRowCount(const uint64_t& aID,
                                              uint32_t* aSelectedRows)
{
  *aSelectedRows = 0;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aSelectedRows = acc->SelectedRowCount();
  }

  return true;
}

bool
DocAccessibleChild::RecvTableSelectedCells(const uint64_t& aID,
                                           nsTArray<uint64_t>* aCellIDs)
{
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    nsAutoTArray<Accessible*, 30> cells;
    acc->SelectedCells(&cells);
    aCellIDs->SetCapacity(cells.Length());
    for (uint32_t i = 0; i < cells.Length(); ++i) {
      aCellIDs->AppendElement(
        reinterpret_cast<uint64_t>(cells[i]->UniqueID()));
    }
  }

  return true;
}

bool
DocAccessibleChild::RecvTableSelectedCellIndices(const uint64_t& aID,
                                                 nsTArray<uint32_t>* aCellIndices)
{
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    acc->SelectedCellIndices(aCellIndices);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableSelectedColumnIndices(const uint64_t& aID,
                                                   nsTArray<uint32_t>* aColumnIndices)
{
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    acc->SelectedColIndices(aColumnIndices);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableSelectedRowIndices(const uint64_t& aID,
                                                nsTArray<uint32_t>* aRowIndices)
{
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    acc->SelectedRowIndices(aRowIndices);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableSelectColumn(const uint64_t& aID,
                                          const uint32_t& aCol)
{
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    acc->SelectCol(aCol);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableSelectRow(const uint64_t& aID,
                                       const uint32_t& aRow)
{
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    acc->SelectRow(aRow);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableUnselectColumn(const uint64_t& aID,
                                            const uint32_t& aCol)
{
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    acc->UnselectCol(aCol);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableUnselectRow(const uint64_t& aID,
                                         const uint32_t& aRow)
{
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    acc->UnselectRow(aRow);
  }

  return true;
}

bool
DocAccessibleChild::RecvTableIsProbablyForLayout(const uint64_t& aID,
                                                 bool* aForLayout)
{
  *aForLayout = false;
  TableAccessible* acc = IdToTableAccessible(aID);
  if (acc) {
    *aForLayout = acc->IsProbablyLayoutTable();
  }

  return true;
}

}
}
