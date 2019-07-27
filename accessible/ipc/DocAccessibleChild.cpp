





#include "DocAccessibleChild.h"

#include "Accessible-inl.h"
#include "ProxyAccessible.h"

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
DocAccessibleChild::IdToAccessible(const uint64_t& aID)
{
  return mDoc->GetAccessibleByUniqueID(reinterpret_cast<void*>(aID));
}

HyperTextAccessible*
DocAccessibleChild::IdToHyperTextAccessible(const uint64_t& aID)
{
  Accessible* acc = IdToAccessible(aID);
  MOZ_ASSERT(!acc || acc->IsHyperText());
  return acc ? acc->AsHyperText() : nullptr;
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
  if (!props)
    return true;

  nsCOMPtr<nsISimpleEnumerator> propEnum;
  nsresult rv = props->Enumerate(getter_AddRefs(propEnum));
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
                                           const AccessibleTextBoundary& aBoundaryType,
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
                                        const AccessibleTextBoundary& aBoundaryType,
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
                                            const AccessibleTextBoundary& aBoundaryType,
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

}
}
