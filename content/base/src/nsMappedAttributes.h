










































#ifndef nsMappedAttributes_h___
#define nsMappedAttributes_h___

#include "nsAttrAndChildArray.h"
#include "nsGenericHTMLElement.h"
#include "nsIStyleRule.h"

class nsIAtom;
class nsHTMLStyleSheet;
class nsRuleWalker;

class nsMappedAttributes : public nsIStyleRule
{
public:
  nsMappedAttributes(nsHTMLStyleSheet* aSheet,
                     nsMapRuleToAttributesFunc aMapRuleFunc);

  void* operator new(size_t size, PRUint32 aAttrCount = 1) CPP_THROW_NEW;

  nsMappedAttributes* Clone(PRBool aWillAddAttr);

  NS_DECL_ISUPPORTS

  nsresult SetAndTakeAttr(nsIAtom* aAttrName, nsAttrValue& aValue);
  const nsAttrValue* GetAttr(nsIAtom* aAttrName) const;

  PRUint32 Count() const
  {
    return mAttrCount;
  }

  PRBool Equals(const nsMappedAttributes* aAttributes) const;
  PRUint32 HashValue() const;

  void DropStyleSheetReference()
  {
    mSheet = nsnull;
  }
  void SetStyleSheet(nsHTMLStyleSheet* aSheet);
  nsHTMLStyleSheet* GetStyleSheet()
  {
    return mSheet;
  }

  const nsAttrName* NameAt(PRUint32 aPos) const
  {
    NS_ASSERTION(aPos < mAttrCount, "out-of-bounds");
    return &Attrs()[aPos].mName;
  }
  const nsAttrValue* AttrAt(PRUint32 aPos) const
  {
    NS_ASSERTION(aPos < mAttrCount, "out-of-bounds");
    return &Attrs()[aPos].mValue;
  }
  
  
  void RemoveAttrAt(PRUint32 aPos, nsAttrValue& aValue);
  const nsAttrName* GetExistingAttrNameFromQName(const nsACString& aName) const;
  PRInt32 IndexOfAttr(nsIAtom* aLocalName, PRInt32 aNamespaceID) const;
  

  
  NS_IMETHOD MapRuleInfoInto(nsRuleData* aRuleData);
#ifdef DEBUG
  NS_METHOD List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

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

  PRUint16 mAttrCount;
#ifdef DEBUG
  PRUint16 mBufferSize;
#endif
  nsHTMLStyleSheet* mSheet; 
  nsMapRuleToAttributesFunc mRuleMapper;
  void* mAttrs[1];
};

#endif 
