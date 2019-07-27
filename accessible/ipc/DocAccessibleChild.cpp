





#include "DocAccessibleChild.h"

#include "Accessible-inl.h"

#include "nsIPersistentProperties2.h"
#include "nsISimpleEnumerator.h"

namespace mozilla {
namespace a11y {

void
SerializeTree(Accessible* aRoot, nsTArray<AccessibleData>& aTree)
{
  uint64_t id = reinterpret_cast<uint64_t>(aRoot->UniqueID());
  uint32_t role = aRoot->Role();
  uint32_t childCount = aRoot->ChildCount();

  
  
  
  if (childCount == 1 && aRoot->GetChildAt(0)->IsDoc())
    childCount = 0;

  aTree.AppendElement(AccessibleData(id, role, childCount));
  for (uint32_t i = 0; i < childCount; i++)
    SerializeTree(aRoot->GetChildAt(i), aTree);
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
  Accessible* acc = mDoc->GetAccessibleByUniqueID((void*)aID);
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
  Accessible* acc = mDoc->GetAccessibleByUniqueID((void*)aID);
  if (!acc)
    return true;

  acc->Name(*aName);
  return true;
}

bool
DocAccessibleChild::RecvDescription(const uint64_t& aID, nsString* aDesc)
{
  Accessible* acc = mDoc->GetAccessibleByUniqueID((void*)aID);
  if (!acc)
    return true;

  acc->Description(*aDesc);
  return true;
}

bool
DocAccessibleChild::RecvAttributes(const uint64_t& aID, nsTArray<Attribute>* aAttributes)
{
  Accessible* acc = mDoc->GetAccessibleByUniqueID((void*)aID);
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
}
}
