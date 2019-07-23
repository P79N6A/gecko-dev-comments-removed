










































#ifndef nsAttrAndChildArray_h___
#define nsAttrAndChildArray_h___

#include "nscore.h"
#include "nsAttrName.h"
#include "nsAttrValue.h"

class nsINode;
class nsIContent;
class nsMappedAttributes;
class nsHTMLStyleSheet;
class nsRuleWalker;
class nsGenericHTMLElement;

#define ATTRCHILD_ARRAY_GROWSIZE 8
#define ATTRCHILD_ARRAY_LINEAR_THRESHOLD 32

#define ATTRCHILD_ARRAY_ATTR_SLOTS_BITS 10

#define ATTRCHILD_ARRAY_MAX_ATTR_COUNT \
    ((1 << ATTRCHILD_ARRAY_ATTR_SLOTS_BITS) - 1)

#define ATTRCHILD_ARRAY_MAX_CHILD_COUNT \
    (~PtrBits(0) >> ATTRCHILD_ARRAY_ATTR_SLOTS_BITS)

#define ATTRCHILD_ARRAY_ATTR_SLOTS_COUNT_MASK \
    ((1 << ATTRCHILD_ARRAY_ATTR_SLOTS_BITS) - 1)


#define ATTRSIZE (sizeof(InternalAttr) / sizeof(void*))

class nsAttrAndChildArray
{
public:
  nsAttrAndChildArray();
  ~nsAttrAndChildArray();

  PRUint32 ChildCount() const
  {
    return mImpl ? (mImpl->mAttrAndChildCount >> ATTRCHILD_ARRAY_ATTR_SLOTS_BITS) : 0;
  }
  nsIContent* ChildAt(PRUint32 aPos) const
  {
    NS_ASSERTION(aPos < ChildCount(), "out-of-bounds access in nsAttrAndChildArray");
    return NS_REINTERPRET_CAST(nsIContent*, mImpl->mBuffer[AttrSlotsSize() + aPos]);
  }
  nsIContent* GetSafeChildAt(PRUint32 aPos) const;
  nsresult AppendChild(nsIContent* aChild)
  {
    return InsertChildAt(aChild, ChildCount());
  }
  nsresult InsertChildAt(nsIContent* aChild, PRUint32 aPos);
  void RemoveChildAt(PRUint32 aPos);
  PRInt32 IndexOfChild(nsINode* aPossibleChild) const;

  PRUint32 AttrCount() const;
  const nsAttrValue* GetAttr(nsIAtom* aLocalName, PRInt32 aNamespaceID = kNameSpaceID_None) const;
  const nsAttrValue* AttrAt(PRUint32 aPos) const;
  nsresult SetAttr(nsIAtom* aLocalName, const nsAString& aValue);
  nsresult SetAndTakeAttr(nsIAtom* aLocalName, nsAttrValue& aValue);
  nsresult SetAndTakeAttr(nsINodeInfo* aName, nsAttrValue& aValue);

  
  
  nsresult RemoveAttrAt(PRUint32 aPos, nsAttrValue& aValue);

  
  const nsAttrName* AttrNameAt(PRUint32 aPos) const;

  
  const nsAttrName* GetSafeAttrNameAt(PRUint32 aPos) const;

  
  const nsAttrName* GetExistingAttrNameFromQName(const nsACString& aName) const;
  PRInt32 IndexOfAttr(nsIAtom* aLocalName, PRInt32 aNamespaceID = kNameSpaceID_None) const;

  nsresult SetAndTakeMappedAttr(nsIAtom* aLocalName, nsAttrValue& aValue,
                                nsGenericHTMLElement* aContent,
                                nsHTMLStyleSheet* aSheet);
  nsresult SetMappedAttrStyleSheet(nsHTMLStyleSheet* aSheet);
  void WalkMappedAttributeStyleRules(nsRuleWalker* aRuleWalker);

  void Compact();

private:
  nsAttrAndChildArray(const nsAttrAndChildArray& aOther); 
  nsAttrAndChildArray& operator=(const nsAttrAndChildArray& aOther); 

  void Clear();

  PRUint32 NonMappedAttrCount() const;
  PRUint32 MappedAttrCount() const;

  nsresult GetModifiableMapped(nsGenericHTMLElement* aContent,
                               nsHTMLStyleSheet* aSheet,
                               PRBool aWillAddAttr,
                               nsMappedAttributes** aModifiable);
  nsresult MakeMappedUnique(nsMappedAttributes* aAttributes);

  PRUint32 AttrSlotsSize() const
  {
    return AttrSlotCount() * ATTRSIZE;
  }

  PRUint32 AttrSlotCount() const
  {
    return mImpl ? mImpl->mAttrAndChildCount & ATTRCHILD_ARRAY_ATTR_SLOTS_COUNT_MASK : 0;
  }

  void SetChildCount(PRUint32 aCount)
  {
    mImpl->mAttrAndChildCount = 
        (mImpl->mAttrAndChildCount & ATTRCHILD_ARRAY_ATTR_SLOTS_COUNT_MASK) |
        (aCount << ATTRCHILD_ARRAY_ATTR_SLOTS_BITS);
  }

  void SetAttrSlotCount(PRUint32 aCount)
  {
    mImpl->mAttrAndChildCount =
        (mImpl->mAttrAndChildCount & ~ATTRCHILD_ARRAY_ATTR_SLOTS_COUNT_MASK) |
        aCount;
  }

  void SetAttrSlotAndChildCount(PRUint32 aSlotCount, PRUint32 aChildCount)
  {
    mImpl->mAttrAndChildCount = aSlotCount |
      (aChildCount << ATTRCHILD_ARRAY_ATTR_SLOTS_BITS);
  }

  PRBool GrowBy(PRUint32 aGrowSize);
  PRBool AddAttrSlot();

  struct InternalAttr
  {
    nsAttrName mName;
    nsAttrValue mValue;
  };

  struct Impl {
    PRUint32 mAttrAndChildCount;
    PRUint32 mBufferSize;
    nsMappedAttributes* mMappedAttrs;
    void* mBuffer[1];
  };

  Impl* mImpl;
};

#endif
