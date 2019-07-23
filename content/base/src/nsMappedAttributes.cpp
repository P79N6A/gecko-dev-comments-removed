










































#include "nsMappedAttributes.h"
#include "nsHTMLStyleSheet.h"
#include "nsRuleWalker.h"
#include "prmem.h"

nsMappedAttributes::nsMappedAttributes(nsHTMLStyleSheet* aSheet,
                                       nsMapRuleToAttributesFunc aMapRuleFunc)
  : mAttrCount(0),
    mSheet(aSheet),
    mRuleMapper(aMapRuleFunc)
{
}

nsMappedAttributes::nsMappedAttributes(const nsMappedAttributes& aCopy)
  : mAttrCount(aCopy.mAttrCount),
    mSheet(aCopy.mSheet),
    mRuleMapper(aCopy.mRuleMapper)
{
  NS_ASSERTION(mBufferSize >= aCopy.mAttrCount, "can't fit attributes");

  PRUint32 i;
  for (i = 0; i < mAttrCount; ++i) {
    new (&Attrs()[i]) InternalAttr(aCopy.Attrs()[i]);
  }
}

nsMappedAttributes::~nsMappedAttributes()
{
  if (mSheet) {
    mSheet->DropMappedAttributes(this);
  }

  PRUint32 i;
  for (i = 0; i < mAttrCount; ++i) {
    Attrs()[i].~InternalAttr();
  }
}


nsMappedAttributes*
nsMappedAttributes::Clone(PRBool aWillAddAttr)
{
  PRUint32 extra = aWillAddAttr ? 1 : 0;

  
  return new (mAttrCount + extra) nsMappedAttributes(*this);
}

void* nsMappedAttributes::operator new(size_t aSize, PRUint32 aAttrCount) CPP_THROW_NEW
{
  NS_ASSERTION(aAttrCount > 0, "zero-attribute nsMappedAttributes requested");

  
  void* newAttrs = ::operator new(aSize - sizeof(void*[1]) +
                                  aAttrCount * sizeof(InternalAttr));

#ifdef DEBUG
  if (newAttrs) {
    static_cast<nsMappedAttributes*>(newAttrs)->mBufferSize = aAttrCount;
  }
#endif

  return newAttrs;
}

NS_IMPL_ISUPPORTS1(nsMappedAttributes,
                   nsIStyleRule)

nsresult
nsMappedAttributes::SetAndTakeAttr(nsIAtom* aAttrName, nsAttrValue& aValue)
{
  NS_PRECONDITION(aAttrName, "null name");

  PRUint32 i;
  for (i = 0; i < mAttrCount && !Attrs()[i].mName.IsSmaller(aAttrName); ++i) {
    if (Attrs()[i].mName.Equals(aAttrName)) {
      Attrs()[i].mValue.Reset();
      Attrs()[i].mValue.SwapValueWith(aValue);

      return NS_OK;
    }
  }

  NS_ASSERTION(mBufferSize >= mAttrCount + 1, "can't fit attributes");

  if (mAttrCount != i) {
    memmove(&Attrs()[i + 1], &Attrs()[i], (mAttrCount - i) * sizeof(InternalAttr));
  }

  new (&Attrs()[i].mName) nsAttrName(aAttrName);
  new (&Attrs()[i].mValue) nsAttrValue();
  Attrs()[i].mValue.SwapValueWith(aValue);
  ++mAttrCount;

  return NS_OK;
}

const nsAttrValue*
nsMappedAttributes::GetAttr(nsIAtom* aAttrName) const
{
  NS_PRECONDITION(aAttrName, "null name");

  PRInt32 i = IndexOfAttr(aAttrName, kNameSpaceID_None);
  if (i >= 0) {
    return &Attrs()[i].mValue;
  }

  return nsnull;
}

PRBool
nsMappedAttributes::Equals(const nsMappedAttributes* aOther) const
{
  if (this == aOther) {
    return PR_TRUE;
  }

  if (mRuleMapper != aOther->mRuleMapper || mAttrCount != aOther->mAttrCount) {
    return PR_FALSE;
  }

  PRUint32 i;
  for (i = 0; i < mAttrCount; ++i) {
    if (!Attrs()[i].mName.Equals(aOther->Attrs()[i].mName) ||
        !Attrs()[i].mValue.Equals(aOther->Attrs()[i].mValue)) {
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}

PRUint32
nsMappedAttributes::HashValue() const
{
  PRUint32 value = NS_PTR_TO_INT32(mRuleMapper);

  PRUint32 i;
  for (i = 0; i < mAttrCount; ++i) {
    value ^= Attrs()[i].mName.HashValue() ^ Attrs()[i].mValue.HashValue();
  }

  return value;
}

void
nsMappedAttributes::SetStyleSheet(nsHTMLStyleSheet* aSheet)
{
  if (mSheet) {
    mSheet->DropMappedAttributes(this);
  }
  mSheet = aSheet;  
}

NS_IMETHODIMP
nsMappedAttributes::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (mRuleMapper) {
    (*mRuleMapper)(this, aRuleData);
  }
  return NS_OK;
}

#ifdef DEBUG
NS_IMETHODIMP
nsMappedAttributes::List(FILE* out, PRInt32 aIndent) const
{
  nsAutoString buffer;
  PRUint32 i;

  for (i = 0; i < mAttrCount; ++i) {
    PRInt32 indent;
    for (indent = aIndent; indent > 0; --indent)
      fputs("  ", out);

    if (Attrs()[i].mName.IsAtom()) {
      Attrs()[i].mName.Atom()->ToString(buffer);
    }
    else {
      Attrs()[i].mName.NodeInfo()->GetQualifiedName(buffer);
    }
    fputs(NS_LossyConvertUTF16toASCII(buffer).get(), out);

    Attrs()[i].mValue.ToString(buffer);
    fputs(NS_LossyConvertUTF16toASCII(buffer).get(), out);
    fputs("\n", out);
  }

  return NS_OK;
}
#endif

void
nsMappedAttributes::RemoveAttrAt(PRUint32 aPos, nsAttrValue& aValue)
{
  Attrs()[aPos].mValue.SwapValueWith(aValue);
  Attrs()[aPos].~InternalAttr();
  memmove(&Attrs()[aPos], &Attrs()[aPos + 1],
          (mAttrCount - aPos - 1) * sizeof(InternalAttr));
  mAttrCount--;
}

const nsAttrName*
nsMappedAttributes::GetExistingAttrNameFromQName(const nsACString& aName) const
{
  PRUint32 i;
  for (i = 0; i < mAttrCount; ++i) {
    if (Attrs()[i].mName.IsAtom()) {
      if (Attrs()[i].mName.Atom()->EqualsUTF8(aName)) {
        return &Attrs()[i].mName;
      }
    }
    else {
      if (Attrs()[i].mName.NodeInfo()->QualifiedNameEquals(aName)) {
        return &Attrs()[i].mName;
      }
    }
  }

  return nsnull;
}

PRInt32
nsMappedAttributes::IndexOfAttr(nsIAtom* aLocalName, PRInt32 aNamespaceID) const
{
  PRUint32 i;
  if (aNamespaceID == kNameSpaceID_None) {
    
    for (i = 0; i < mAttrCount; ++i) {
      if (Attrs()[i].mName.Equals(aLocalName)) {
        return i;
      }
    }
  }
  else {
    for (i = 0; i < mAttrCount; ++i) {
      if (Attrs()[i].mName.Equals(aLocalName, aNamespaceID)) {
        return i;
      }
    }
  }

  return -1;
}
