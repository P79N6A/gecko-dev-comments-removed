









#include "nsAttrAndChildArray.h"
#include "nsMappedAttributeElement.h"
#include "prmem.h"
#include "prbit.h"
#include "nsString.h"
#include "nsHTMLStyleSheet.h"
#include "nsRuleWalker.h"
#include "nsMappedAttributes.h"
#include "nsUnicharUtils.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h" 















#define CACHE_POINTER_SHIFT 5
#define CACHE_NUM_SLOTS 128
#define CACHE_CHILD_LIMIT 10

#define CACHE_GET_INDEX(_array) \
  ((NS_PTR_TO_INT32(_array) >> CACHE_POINTER_SHIFT) & \
   (CACHE_NUM_SLOTS - 1))

struct IndexCacheSlot
{
  const nsAttrAndChildArray* array;
  PRInt32 index;
};




static IndexCacheSlot indexCache[CACHE_NUM_SLOTS];

static
inline
void
AddIndexToCache(const nsAttrAndChildArray* aArray, PRInt32 aIndex)
{
  PRUint32 ix = CACHE_GET_INDEX(aArray);
  indexCache[ix].array = aArray;
  indexCache[ix].index = aIndex;
}

static
inline
PRInt32
GetIndexFromCache(const nsAttrAndChildArray* aArray)
{
  PRUint32 ix = CACHE_GET_INDEX(aArray);
  return indexCache[ix].array == aArray ? indexCache[ix].index : -1;
}









#define ATTRS(_impl) \
  reinterpret_cast<InternalAttr*>(&((_impl)->mBuffer[0]))
  

#define NS_IMPL_EXTRA_SIZE \
  ((sizeof(Impl) - sizeof(mImpl->mBuffer)) / sizeof(void*))

nsAttrAndChildArray::nsAttrAndChildArray()
  : mImpl(nsnull)
{
}

nsAttrAndChildArray::~nsAttrAndChildArray()
{
  if (!mImpl) {
    return;
  }

  Clear();

  PR_Free(mImpl);
}

nsIContent*
nsAttrAndChildArray::GetSafeChildAt(PRUint32 aPos) const
{
  if (aPos < ChildCount()) {
    return ChildAt(aPos);
  }
  
  return nsnull;
}

nsIContent * const *
nsAttrAndChildArray::GetChildArray(PRUint32* aChildCount) const
{
  *aChildCount = ChildCount();
  
  if (!*aChildCount) {
    return nsnull;
  }
  
  return reinterpret_cast<nsIContent**>(mImpl->mBuffer + AttrSlotsSize());
}

nsresult
nsAttrAndChildArray::InsertChildAt(nsIContent* aChild, PRUint32 aPos)
{
  NS_ASSERTION(aChild, "nullchild");
  NS_ASSERTION(aPos <= ChildCount(), "out-of-bounds");

  PRUint32 offset = AttrSlotsSize();
  PRUint32 childCount = ChildCount();

  NS_ENSURE_TRUE(childCount < ATTRCHILD_ARRAY_MAX_CHILD_COUNT,
                 NS_ERROR_FAILURE);

  
  if (mImpl && offset + childCount < mImpl->mBufferSize) {
    void** pos = mImpl->mBuffer + offset + aPos;
    if (childCount != aPos) {
      memmove(pos + 1, pos, (childCount - aPos) * sizeof(nsIContent*));
    }
    SetChildAtPos(pos, aChild, aPos, childCount);

    SetChildCount(childCount + 1);

    return NS_OK;
  }

  
  if (offset && !mImpl->mBuffer[offset - ATTRSIZE]) {
    
    
    PRUint32 attrCount = NonMappedAttrCount();
    void** newStart = mImpl->mBuffer + attrCount * ATTRSIZE;
    void** oldStart = mImpl->mBuffer + offset;
    memmove(newStart, oldStart, aPos * sizeof(nsIContent*));
    memmove(&newStart[aPos + 1], &oldStart[aPos],
            (childCount - aPos) * sizeof(nsIContent*));
    SetChildAtPos(newStart + aPos, aChild, aPos, childCount);

    SetAttrSlotAndChildCount(attrCount, childCount + 1);

    return NS_OK;
  }

  
  if (!GrowBy(1)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  void** pos = mImpl->mBuffer + offset + aPos;
  if (childCount != aPos) {
    memmove(pos + 1, pos, (childCount - aPos) * sizeof(nsIContent*));
  }
  SetChildAtPos(pos, aChild, aPos, childCount);

  SetChildCount(childCount + 1);
  
  return NS_OK;
}

void
nsAttrAndChildArray::RemoveChildAt(PRUint32 aPos)
{
  
  
  nsCOMPtr<nsIContent> child = TakeChildAt(aPos);
}

already_AddRefed<nsIContent>
nsAttrAndChildArray::TakeChildAt(PRUint32 aPos)
{
  NS_ASSERTION(aPos < ChildCount(), "out-of-bounds");

  PRUint32 childCount = ChildCount();
  void** pos = mImpl->mBuffer + AttrSlotsSize() + aPos;
  nsIContent* child = static_cast<nsIContent*>(*pos);
  if (child->mPreviousSibling) {
    child->mPreviousSibling->mNextSibling = child->mNextSibling;
  }
  if (child->mNextSibling) {
    child->mNextSibling->mPreviousSibling = child->mPreviousSibling;
  }
  child->mPreviousSibling = child->mNextSibling = nsnull;

  memmove(pos, pos + 1, (childCount - aPos - 1) * sizeof(nsIContent*));
  SetChildCount(childCount - 1);

  return child;
}

PRInt32
nsAttrAndChildArray::IndexOfChild(nsINode* aPossibleChild) const
{
  if (!mImpl) {
    return -1;
  }
  void** children = mImpl->mBuffer + AttrSlotsSize();
  
  PRInt32 i, count = ChildCount();

  if (count >= CACHE_CHILD_LIMIT) {
    PRInt32 cursor = GetIndexFromCache(this);
    
    
    
    
    if (cursor >= count) {
      cursor = -1;
    }

    
    
    
    PRInt32 inc = 1, sign = 1;
    while (cursor >= 0 && cursor < count) {
      if (children[cursor] == aPossibleChild) {
        AddIndexToCache(this, cursor);

        return cursor;
      }

      cursor += inc;
      inc = -inc - sign;
      sign = -sign;
    }

    
    
    
    cursor += inc;

    if (sign > 0) {
      for (; cursor < count; ++cursor) {
        if (children[cursor] == aPossibleChild) {
          AddIndexToCache(this, cursor);

          return static_cast<PRInt32>(cursor);
        }
      }
    }
    else {
      for (; cursor >= 0; --cursor) {
        if (children[cursor] == aPossibleChild) {
          AddIndexToCache(this, cursor);

          return static_cast<PRInt32>(cursor);
        }
      }
    }

    
    return -1;
  }

  for (i = 0; i < count; ++i) {
    if (children[i] == aPossibleChild) {
      return static_cast<PRInt32>(i);
    }
  }

  return -1;
}

PRUint32
nsAttrAndChildArray::AttrCount() const
{
  return NonMappedAttrCount() + MappedAttrCount();
}

const nsAttrValue*
nsAttrAndChildArray::GetAttr(nsIAtom* aLocalName, PRInt32 aNamespaceID) const
{
  PRUint32 i, slotCount = AttrSlotCount();
  if (aNamespaceID == kNameSpaceID_None) {
    
    for (i = 0; i < slotCount && AttrSlotIsTaken(i); ++i) {
      if (ATTRS(mImpl)[i].mName.Equals(aLocalName)) {
        return &ATTRS(mImpl)[i].mValue;
      }
    }

    if (mImpl && mImpl->mMappedAttrs) {
      return mImpl->mMappedAttrs->GetAttr(aLocalName);
    }
  }
  else {
    for (i = 0; i < slotCount && AttrSlotIsTaken(i); ++i) {
      if (ATTRS(mImpl)[i].mName.Equals(aLocalName, aNamespaceID)) {
        return &ATTRS(mImpl)[i].mValue;
      }
    }
  }

  return nsnull;
}

const nsAttrValue*
nsAttrAndChildArray::GetAttr(const nsAString& aName,
                             nsCaseTreatment aCaseSensitive) const
{
  PRUint32 i, slotCount = AttrSlotCount();
  for (i = 0; i < slotCount && AttrSlotIsTaken(i); ++i) {
    if (ATTRS(mImpl)[i].mName.QualifiedNameEquals(aName)) {
      return &ATTRS(mImpl)[i].mValue;
    }
  }

  if (mImpl && mImpl->mMappedAttrs) {
    const nsAttrValue* val =
      mImpl->mMappedAttrs->GetAttr(aName);
    if (val) {
      return val;
    }
  }

  
  
  if (aCaseSensitive == eIgnoreCase &&
      nsContentUtils::StringContainsASCIIUpper(aName)) {
    
    
    nsAutoString lowercase;
    nsContentUtils::ASCIIToLower(aName, lowercase);
    return GetAttr(lowercase, eCaseMatters);
  }

  return nsnull;
}

const nsAttrValue*
nsAttrAndChildArray::AttrAt(PRUint32 aPos) const
{
  NS_ASSERTION(aPos < AttrCount(),
               "out-of-bounds access in nsAttrAndChildArray");

  PRUint32 mapped = MappedAttrCount();
  if (aPos < mapped) {
    return mImpl->mMappedAttrs->AttrAt(aPos);
  }

  return &ATTRS(mImpl)[aPos - mapped].mValue;
}

nsresult
nsAttrAndChildArray::SetAndTakeAttr(nsIAtom* aLocalName, nsAttrValue& aValue)
{
  PRUint32 i, slotCount = AttrSlotCount();
  for (i = 0; i < slotCount && AttrSlotIsTaken(i); ++i) {
    if (ATTRS(mImpl)[i].mName.Equals(aLocalName)) {
      ATTRS(mImpl)[i].mValue.Reset();
      ATTRS(mImpl)[i].mValue.SwapValueWith(aValue);

      return NS_OK;
    }
  }

  NS_ENSURE_TRUE(i < ATTRCHILD_ARRAY_MAX_ATTR_COUNT,
                 NS_ERROR_FAILURE);

  if (i == slotCount && !AddAttrSlot()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  new (&ATTRS(mImpl)[i].mName) nsAttrName(aLocalName);
  new (&ATTRS(mImpl)[i].mValue) nsAttrValue();
  ATTRS(mImpl)[i].mValue.SwapValueWith(aValue);

  return NS_OK;
}

nsresult
nsAttrAndChildArray::SetAndTakeAttr(nsINodeInfo* aName, nsAttrValue& aValue)
{
  PRInt32 namespaceID = aName->NamespaceID();
  nsIAtom* localName = aName->NameAtom();
  if (namespaceID == kNameSpaceID_None) {
    return SetAndTakeAttr(localName, aValue);
  }

  PRUint32 i, slotCount = AttrSlotCount();
  for (i = 0; i < slotCount && AttrSlotIsTaken(i); ++i) {
    if (ATTRS(mImpl)[i].mName.Equals(localName, namespaceID)) {
      ATTRS(mImpl)[i].mName.SetTo(aName);
      ATTRS(mImpl)[i].mValue.Reset();
      ATTRS(mImpl)[i].mValue.SwapValueWith(aValue);

      return NS_OK;
    }
  }

  NS_ENSURE_TRUE(i < ATTRCHILD_ARRAY_MAX_ATTR_COUNT,
                 NS_ERROR_FAILURE);

  if (i == slotCount && !AddAttrSlot()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  new (&ATTRS(mImpl)[i].mName) nsAttrName(aName);
  new (&ATTRS(mImpl)[i].mValue) nsAttrValue();
  ATTRS(mImpl)[i].mValue.SwapValueWith(aValue);

  return NS_OK;
}


nsresult
nsAttrAndChildArray::RemoveAttrAt(PRUint32 aPos, nsAttrValue& aValue)
{
  NS_ASSERTION(aPos < AttrCount(), "out-of-bounds");

  PRUint32 mapped = MappedAttrCount();
  if (aPos < mapped) {
    if (mapped == 1) {
      
      
      aValue.SetTo(*mImpl->mMappedAttrs->AttrAt(0));
      NS_RELEASE(mImpl->mMappedAttrs);

      return NS_OK;
    }

    nsRefPtr<nsMappedAttributes> mapped;
    nsresult rv = GetModifiableMapped(nsnull, nsnull, false,
                                      getter_AddRefs(mapped));
    NS_ENSURE_SUCCESS(rv, rv);

    mapped->RemoveAttrAt(aPos, aValue);

    return MakeMappedUnique(mapped);
  }

  aPos -= mapped;
  ATTRS(mImpl)[aPos].mValue.SwapValueWith(aValue);
  ATTRS(mImpl)[aPos].~InternalAttr();

  PRUint32 slotCount = AttrSlotCount();
  memmove(&ATTRS(mImpl)[aPos],
          &ATTRS(mImpl)[aPos + 1],
          (slotCount - aPos - 1) * sizeof(InternalAttr));
  memset(&ATTRS(mImpl)[slotCount - 1], nsnull, sizeof(InternalAttr));

  return NS_OK;
}

const nsAttrName*
nsAttrAndChildArray::AttrNameAt(PRUint32 aPos) const
{
  NS_ASSERTION(aPos < AttrCount(),
               "out-of-bounds access in nsAttrAndChildArray");

  PRUint32 mapped = MappedAttrCount();
  if (aPos < mapped) {
    return mImpl->mMappedAttrs->NameAt(aPos);
  }

  return &ATTRS(mImpl)[aPos - mapped].mName;
}

const nsAttrName*
nsAttrAndChildArray::GetSafeAttrNameAt(PRUint32 aPos) const
{
  PRUint32 mapped = MappedAttrCount();
  if (aPos < mapped) {
    return mImpl->mMappedAttrs->NameAt(aPos);
  }

  aPos -= mapped;
  if (aPos >= AttrSlotCount()) {
    return nsnull;
  }

  void** pos = mImpl->mBuffer + aPos * ATTRSIZE;
  if (!*pos) {
    return nsnull;
  }

  return &reinterpret_cast<InternalAttr*>(pos)->mName;
}

const nsAttrName*
nsAttrAndChildArray::GetExistingAttrNameFromQName(const nsAString& aName) const
{
  PRUint32 i, slotCount = AttrSlotCount();
  for (i = 0; i < slotCount && AttrSlotIsTaken(i); ++i) {
    if (ATTRS(mImpl)[i].mName.QualifiedNameEquals(aName)) {
      return &ATTRS(mImpl)[i].mName;
    }
  }

  if (mImpl && mImpl->mMappedAttrs) {
    return mImpl->mMappedAttrs->GetExistingAttrNameFromQName(aName);
  }

  return nsnull;
}

PRInt32
nsAttrAndChildArray::IndexOfAttr(nsIAtom* aLocalName, PRInt32 aNamespaceID) const
{
  PRInt32 idx;
  if (mImpl && mImpl->mMappedAttrs && aNamespaceID == kNameSpaceID_None) {
    idx = mImpl->mMappedAttrs->IndexOfAttr(aLocalName);
    if (idx >= 0) {
      return idx;
    }
  }

  PRUint32 i;
  PRUint32 mapped = MappedAttrCount();
  PRUint32 slotCount = AttrSlotCount();
  if (aNamespaceID == kNameSpaceID_None) {
    
    for (i = 0; i < slotCount && AttrSlotIsTaken(i); ++i) {
      if (ATTRS(mImpl)[i].mName.Equals(aLocalName)) {
        return i + mapped;
      }
    }
  }
  else {
    for (i = 0; i < slotCount && AttrSlotIsTaken(i); ++i) {
      if (ATTRS(mImpl)[i].mName.Equals(aLocalName, aNamespaceID)) {
        return i + mapped;
      }
    }
  }

  return -1;
}

nsresult
nsAttrAndChildArray::SetAndTakeMappedAttr(nsIAtom* aLocalName,
                                          nsAttrValue& aValue,
                                          nsMappedAttributeElement* aContent,
                                          nsHTMLStyleSheet* aSheet)
{
  nsRefPtr<nsMappedAttributes> mapped;

  bool willAdd = true;
  if (mImpl && mImpl->mMappedAttrs) {
    willAdd = mImpl->mMappedAttrs->GetAttr(aLocalName) == nsnull;
  }

  nsresult rv = GetModifiableMapped(aContent, aSheet, willAdd,
                                    getter_AddRefs(mapped));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mapped->SetAndTakeAttr(aLocalName, aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  return MakeMappedUnique(mapped);
}

nsresult
nsAttrAndChildArray::DoSetMappedAttrStyleSheet(nsHTMLStyleSheet* aSheet)
{
  NS_PRECONDITION(mImpl && mImpl->mMappedAttrs,
                  "Should have mapped attrs here!");
  if (aSheet == mImpl->mMappedAttrs->GetStyleSheet()) {
    return NS_OK;
  }

  nsRefPtr<nsMappedAttributes> mapped;
  nsresult rv = GetModifiableMapped(nsnull, nsnull, false, 
                                    getter_AddRefs(mapped));
  NS_ENSURE_SUCCESS(rv, rv);

  mapped->SetStyleSheet(aSheet);

  return MakeMappedUnique(mapped);
}

void
nsAttrAndChildArray::WalkMappedAttributeStyleRules(nsRuleWalker* aRuleWalker)
{
  if (mImpl && mImpl->mMappedAttrs) {
    aRuleWalker->Forward(mImpl->mMappedAttrs);
  }
}

void
nsAttrAndChildArray::Compact()
{
  if (!mImpl) {
    return;
  }

  
  PRUint32 slotCount = AttrSlotCount();
  PRUint32 attrCount = NonMappedAttrCount();
  PRUint32 childCount = ChildCount();

  if (attrCount < slotCount) {
    memmove(mImpl->mBuffer + attrCount * ATTRSIZE,
            mImpl->mBuffer + slotCount * ATTRSIZE,
            childCount * sizeof(nsIContent*));
    SetAttrSlotCount(attrCount);
  }

  
  PRUint32 newSize = attrCount * ATTRSIZE + childCount;
  if (!newSize && !mImpl->mMappedAttrs) {
    PR_Free(mImpl);
    mImpl = nsnull;
  }
  else if (newSize < mImpl->mBufferSize) {
    mImpl = static_cast<Impl*>(PR_Realloc(mImpl, (newSize + NS_IMPL_EXTRA_SIZE) * sizeof(nsIContent*)));
    NS_ASSERTION(mImpl, "failed to reallocate to smaller buffer");

    mImpl->mBufferSize = newSize;
  }
}

void
nsAttrAndChildArray::Clear()
{
  if (!mImpl) {
    return;
  }

  if (mImpl->mMappedAttrs) {
    NS_RELEASE(mImpl->mMappedAttrs);
  }

  PRUint32 i, slotCount = AttrSlotCount();
  for (i = 0; i < slotCount && AttrSlotIsTaken(i); ++i) {
    ATTRS(mImpl)[i].~InternalAttr();
  }

  nsAutoScriptBlocker scriptBlocker;
  PRUint32 end = slotCount * ATTRSIZE + ChildCount();
  for (i = slotCount * ATTRSIZE; i < end; ++i) {
    nsIContent* child = static_cast<nsIContent*>(mImpl->mBuffer[i]);
    
    
    child->UnbindFromTree(false); 
    
    

    
    
    
    
    
    
    
    
    child->mPreviousSibling = child->mNextSibling = nsnull;
    NS_RELEASE(child);
  }

  SetAttrSlotAndChildCount(0, 0);
}

PRUint32
nsAttrAndChildArray::NonMappedAttrCount() const
{
  if (!mImpl) {
    return 0;
  }

  PRUint32 count = AttrSlotCount();
  while (count > 0 && !mImpl->mBuffer[(count - 1) * ATTRSIZE]) {
    --count;
  }

  return count;
}

PRUint32
nsAttrAndChildArray::MappedAttrCount() const
{
  return mImpl && mImpl->mMappedAttrs ? (PRUint32)mImpl->mMappedAttrs->Count() : 0;
}

nsresult
nsAttrAndChildArray::GetModifiableMapped(nsMappedAttributeElement* aContent,
                                         nsHTMLStyleSheet* aSheet,
                                         bool aWillAddAttr,
                                         nsMappedAttributes** aModifiable)
{
  *aModifiable = nsnull;

  if (mImpl && mImpl->mMappedAttrs) {
    *aModifiable = mImpl->mMappedAttrs->Clone(aWillAddAttr);
    NS_ENSURE_TRUE(*aModifiable, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aModifiable);
    
    return NS_OK;
  }

  NS_ASSERTION(aContent, "Trying to create modifiable without content");

  nsMapRuleToAttributesFunc mapRuleFunc =
    aContent->GetAttributeMappingFunction();
  *aModifiable = new nsMappedAttributes(aSheet, mapRuleFunc);
  NS_ENSURE_TRUE(*aModifiable, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aModifiable);

  return NS_OK;
}

nsresult
nsAttrAndChildArray::MakeMappedUnique(nsMappedAttributes* aAttributes)
{
  NS_ASSERTION(aAttributes, "missing attributes");

  if (!mImpl && !GrowBy(1)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!aAttributes->GetStyleSheet()) {
    

    nsRefPtr<nsMappedAttributes> mapped(aAttributes);
    mapped.swap(mImpl->mMappedAttrs);

    return NS_OK;
  }

  nsRefPtr<nsMappedAttributes> mapped =
    aAttributes->GetStyleSheet()->UniqueMappedAttributes(aAttributes);
  NS_ENSURE_TRUE(mapped, NS_ERROR_OUT_OF_MEMORY);

  if (mapped != aAttributes) {
    
    
    
    
    
    aAttributes->DropStyleSheetReference();
  }
  mapped.swap(mImpl->mMappedAttrs);

  return NS_OK;
}


bool
nsAttrAndChildArray::GrowBy(PRUint32 aGrowSize)
{
  PRUint32 size = mImpl ? mImpl->mBufferSize + NS_IMPL_EXTRA_SIZE : 0;
  PRUint32 minSize = size + aGrowSize;

  if (minSize <= ATTRCHILD_ARRAY_LINEAR_THRESHOLD) {
    do {
      size += ATTRCHILD_ARRAY_GROWSIZE;
    } while (size < minSize);
  }
  else {
    size = PR_BIT(PR_CeilingLog2(minSize));
  }

  bool needToInitialize = !mImpl;
  Impl* newImpl = static_cast<Impl*>(PR_Realloc(mImpl, size * sizeof(void*)));
  NS_ENSURE_TRUE(newImpl, false);

  mImpl = newImpl;

  
  if (needToInitialize) {
    mImpl->mMappedAttrs = nsnull;
    SetAttrSlotAndChildCount(0, 0);
  }

  mImpl->mBufferSize = size - NS_IMPL_EXTRA_SIZE;

  return true;
}

bool
nsAttrAndChildArray::AddAttrSlot()
{
  PRUint32 slotCount = AttrSlotCount();
  PRUint32 childCount = ChildCount();

  
  if (!(mImpl && mImpl->mBufferSize >= (slotCount + 1) * ATTRSIZE + childCount) &&
      !GrowBy(ATTRSIZE)) {
    return false;
  }
  void** offset = mImpl->mBuffer + slotCount * ATTRSIZE;

  if (childCount > 0) {
    memmove(&ATTRS(mImpl)[slotCount + 1], &ATTRS(mImpl)[slotCount],
            childCount * sizeof(nsIContent*));
  }

  SetAttrSlotCount(slotCount + 1);
  offset[0] = nsnull;
  offset[1] = nsnull;

  return true;
}

inline void
nsAttrAndChildArray::SetChildAtPos(void** aPos, nsIContent* aChild,
                                   PRUint32 aIndex, PRUint32 aChildCount)
{
  NS_PRECONDITION(!aChild->GetNextSibling(), "aChild with next sibling?");
  NS_PRECONDITION(!aChild->GetPreviousSibling(), "aChild with prev sibling?");

  *aPos = aChild;
  NS_ADDREF(aChild);
  if (aIndex != 0) {
    nsIContent* previous = static_cast<nsIContent*>(*(aPos - 1));
    aChild->mPreviousSibling = previous;
    previous->mNextSibling = aChild;
  }
  if (aIndex != aChildCount) {
    nsIContent* next = static_cast<nsIContent*>(*(aPos + 1));
    aChild->mNextSibling = next;
    next->mPreviousSibling = aChild;
  }
}

size_t
nsAttrAndChildArray::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  size_t n = 0;
  if (mImpl) {
    

    n += aMallocSizeOf(mImpl);

    PRUint32 slotCount = AttrSlotCount();
    for (PRUint32 i = 0; i < slotCount && AttrSlotIsTaken(i); ++i) {
      nsAttrValue* value = &ATTRS(mImpl)[i].mValue;
      n += value->SizeOfExcludingThis(aMallocSizeOf);
    }
  }

  return n;
}

