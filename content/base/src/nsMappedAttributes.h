









#ifndef nsMappedAttributes_h___
#define nsMappedAttributes_h___

#include "nsAttrAndChildArray.h"
#include "nsMappedAttributeElement.h"
#include "nsIStyleRule.h"
#include "mozilla/Attributes.h"

class nsIAtom;
class nsHTMLStyleSheet;
class nsRuleWalker;

class nsMappedAttributes MOZ_FINAL : public nsIStyleRule
{
public:
  nsMappedAttributes(nsHTMLStyleSheet* aSheet,
                     nsMapRuleToAttributesFunc aMapRuleFunc);

  
  void* operator new(size_t size, uint32_t aAttrCount = 1) CPP_THROW_NEW;
  nsMappedAttributes* Clone(bool aWillAddAttr);

  NS_DECL_ISUPPORTS

  void SetAndTakeAttr(nsIAtom* aAttrName, nsAttrValue& aValue);
  const nsAttrValue* GetAttr(nsIAtom* aAttrName) const;
  const nsAttrValue* GetAttr(const nsAString& aAttrName) const;

  uint32_t Count() const
  {
    return mAttrCount;
  }

  bool Equals(const nsMappedAttributes* aAttributes) const;
  uint32_t HashValue() const;

  void DropStyleSheetReference()
  {
    mSheet = nullptr;
  }
  void SetStyleSheet(nsHTMLStyleSheet* aSheet);
  nsHTMLStyleSheet* GetStyleSheet()
  {
    return mSheet;
  }

  const nsAttrName* NameAt(uint32_t aPos) const
  {
    NS_ASSERTION(aPos < mAttrCount, "out-of-bounds");
    return &Attrs()[aPos].mName;
  }
  const nsAttrValue* AttrAt(uint32_t aPos) const
  {
    NS_ASSERTION(aPos < mAttrCount, "out-of-bounds");
    return &Attrs()[aPos].mValue;
  }
  
  
  void RemoveAttrAt(uint32_t aPos, nsAttrValue& aValue);
  const nsAttrName* GetExistingAttrNameFromQName(const nsAString& aName) const;
  int32_t IndexOfAttr(nsIAtom* aLocalName) const;
  

  
  virtual void MapRuleInfoInto(nsRuleData* aRuleData) MOZ_OVERRIDE;
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

private:
  nsMappedAttributes(const nsMappedAttributes& aCopy);
  ~nsMappedAttributes();

  struct InternalAttr
  {
    nsAttrName mName;
    nsAttrValue mValue;
  };

  






  const InternalAttr* Attrs() const
  {
    return reinterpret_cast<const InternalAttr*>(&(mAttrs[0]));
  }
  InternalAttr* Attrs()
  {
    return reinterpret_cast<InternalAttr*>(&(mAttrs[0]));
  }

  uint16_t mAttrCount;
#ifdef DEBUG
  uint16_t mBufferSize;
#endif
  nsHTMLStyleSheet* mSheet; 
  nsMapRuleToAttributesFunc mRuleMapper;
  void* mAttrs[1];
};

#endif 
