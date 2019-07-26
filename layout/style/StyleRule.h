









#ifndef mozilla_css_StyleRule_h__
#define mozilla_css_StyleRule_h__

#include "mozilla/Attributes.h"

#include "mozilla/Attributes.h"
#include "mozilla/css/Rule.h"

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSPseudoClasses.h"
#include "nsAutoPtr.h"

class nsIAtom;
class nsCSSStyleSheet;
struct nsCSSSelectorList;
class nsCSSCompressedDataBlock;

struct nsAtomList {
public:
  nsAtomList(nsIAtom* aAtom);
  nsAtomList(const nsString& aAtomValue);
  ~nsAtomList(void);

  
  nsAtomList* Clone() const { return Clone(true); }

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  nsCOMPtr<nsIAtom> mAtom;
  nsAtomList*       mNext;
private: 
  nsAtomList* Clone(bool aDeep) const;

  nsAtomList(const nsAtomList& aCopy) MOZ_DELETE;
  nsAtomList& operator=(const nsAtomList& aCopy) MOZ_DELETE;
};

struct nsPseudoClassList {
public:
  nsPseudoClassList(nsCSSPseudoClasses::Type aType);
  nsPseudoClassList(nsCSSPseudoClasses::Type aType, const PRUnichar *aString);
  nsPseudoClassList(nsCSSPseudoClasses::Type aType, const int32_t *aIntPair);
  nsPseudoClassList(nsCSSPseudoClasses::Type aType,
                    nsCSSSelectorList *aSelectorList );
  ~nsPseudoClassList(void);

  
  nsPseudoClassList* Clone() const { return Clone(true); }

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  union {
    
    
    
    
    
    
    
    
    
    void*           mMemory; 
    PRUnichar*      mString;
    int32_t*        mNumbers;
    nsCSSSelectorList* mSelectors;
  } u;
  nsCSSPseudoClasses::Type mType;
  nsPseudoClassList* mNext;
private: 
  nsPseudoClassList* Clone(bool aDeep) const;

  nsPseudoClassList(const nsPseudoClassList& aCopy) MOZ_DELETE;
  nsPseudoClassList& operator=(const nsPseudoClassList& aCopy) MOZ_DELETE;
};

#define NS_ATTR_FUNC_SET        0     // [attr]
#define NS_ATTR_FUNC_EQUALS     1     // [attr=value]
#define NS_ATTR_FUNC_INCLUDES   2     // [attr~=value] (space separated)
#define NS_ATTR_FUNC_DASHMATCH  3     // [attr|=value] ('-' truncated)
#define NS_ATTR_FUNC_BEGINSMATCH  4   // [attr^=value] (begins with)
#define NS_ATTR_FUNC_ENDSMATCH  5     // [attr$=value] (ends with)
#define NS_ATTR_FUNC_CONTAINSMATCH 6  // [attr*=value] (contains substring)

struct nsAttrSelector {
public:
  nsAttrSelector(int32_t aNameSpace, const nsString& aAttr);
  nsAttrSelector(int32_t aNameSpace, const nsString& aAttr, uint8_t aFunction, 
                 const nsString& aValue, bool aCaseSensitive);
  nsAttrSelector(int32_t aNameSpace, nsIAtom* aLowercaseAttr, 
                 nsIAtom* aCasedAttr, uint8_t aFunction, 
                 const nsString& aValue, bool aCaseSensitive);
  ~nsAttrSelector(void);

  
  nsAttrSelector* Clone() const { return Clone(true); }

  nsString        mValue;
  nsAttrSelector* mNext;
  nsCOMPtr<nsIAtom> mLowercaseAttr;
  nsCOMPtr<nsIAtom> mCasedAttr;
  int32_t         mNameSpace;
  uint8_t         mFunction;
  bool            mCaseSensitive; 
                                  
private: 
  nsAttrSelector* Clone(bool aDeep) const;

  nsAttrSelector(const nsAttrSelector& aCopy) MOZ_DELETE;
  nsAttrSelector& operator=(const nsAttrSelector& aCopy) MOZ_DELETE;
};

struct nsCSSSelector {
public:
  nsCSSSelector(void);
  ~nsCSSSelector(void);

  
  nsCSSSelector* Clone() const { return Clone(true, true); }

  void Reset(void);
  void SetNameSpace(int32_t aNameSpace);
  void SetTag(const nsString& aTag);
  void AddID(const nsString& aID);
  void AddClass(const nsString& aClass);
  void AddPseudoClass(nsCSSPseudoClasses::Type aType);
  void AddPseudoClass(nsCSSPseudoClasses::Type aType, const PRUnichar* aString);
  void AddPseudoClass(nsCSSPseudoClasses::Type aType, const int32_t* aIntPair);
  
  void AddPseudoClass(nsCSSPseudoClasses::Type aType,
                      nsCSSSelectorList* aSelectorList);
  void AddAttribute(int32_t aNameSpace, const nsString& aAttr);
  void AddAttribute(int32_t aNameSpace, const nsString& aAttr, uint8_t aFunc, 
                    const nsString& aValue, bool aCaseSensitive);
  void SetOperator(PRUnichar aOperator);

  inline bool HasTagSelector() const {
    return !!mCasedTag;
  }

  inline bool IsPseudoElement() const {
    return mLowercaseTag && !mCasedTag;
  }

  
  int32_t CalcWeight() const;

  void ToString(nsAString& aString, nsCSSStyleSheet* aSheet,
                bool aAppend = false) const;

private:
  void AddPseudoClassInternal(nsPseudoClassList *aPseudoClass);
  nsCSSSelector* Clone(bool aDeepNext, bool aDeepNegations) const;

  void AppendToStringWithoutCombinators(nsAString& aString,
                                        nsCSSStyleSheet* aSheet) const;
  void AppendToStringWithoutCombinatorsOrNegations(nsAString& aString,
                                                   nsCSSStyleSheet* aSheet,
                                                   bool aIsNegated)
                                                        const;
  
  
  
  bool CanBeNamespaced(bool aIsNegated) const;
  
  
  int32_t CalcWeightWithoutNegations() const;

public:
  
  nsCSSPseudoElements::Type PseudoType() const {
    return static_cast<nsCSSPseudoElements::Type>(mPseudoType);
  }
  void SetPseudoType(nsCSSPseudoElements::Type aType) {
    NS_ASSERTION(aType > INT16_MIN && aType < INT16_MAX, "Out of bounds");
    mPseudoType = static_cast<int16_t>(aType);
  }

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  
  
  
  
  nsCOMPtr<nsIAtom> mLowercaseTag;
  nsCOMPtr<nsIAtom> mCasedTag;
  nsAtomList*     mIDList;
  nsAtomList*     mClassList;
  nsPseudoClassList* mPseudoClassList; 
                                       
  nsAttrSelector* mAttrList;
  nsCSSSelector*  mNegations;
  nsCSSSelector*  mNext;
  int32_t         mNameSpace;
  PRUnichar       mOperator;
private:
  
  int16_t        mPseudoType;

  nsCSSSelector(const nsCSSSelector& aCopy) MOZ_DELETE;
  nsCSSSelector& operator=(const nsCSSSelector& aCopy) MOZ_DELETE;
};







struct nsCSSSelectorList {
  nsCSSSelectorList(void);
  ~nsCSSSelectorList(void);

  








  nsCSSSelector* AddSelector(PRUnichar aOperator);

  


  void ToString(nsAString& aResult, nsCSSStyleSheet* aSheet);

  


  nsCSSSelectorList* Clone() const { return Clone(true); }

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  nsCSSSelector*     mSelectors;
  int32_t            mWeight;
  nsCSSSelectorList* mNext;
private: 
  nsCSSSelectorList* Clone(bool aDeep) const;

  nsCSSSelectorList(const nsCSSSelectorList& aCopy) MOZ_DELETE;
  nsCSSSelectorList& operator=(const nsCSSSelectorList& aCopy) MOZ_DELETE;
};


#define NS_CSS_STYLE_RULE_IMPL_CID \
{ 0x464bab7a, 0x2fce, 0x4f30, \
  { 0xab, 0x44, 0xb7, 0xa5, 0xf3, 0xaa, 0xe5, 0x7d } }

namespace mozilla {
namespace css {

class Declaration;
class DOMCSSStyleRule;

class StyleRule;

class ImportantRule : public nsIStyleRule {
public:
  ImportantRule(Declaration *aDeclaration);

  NS_DECL_ISUPPORTS

  
  virtual void MapRuleInfoInto(nsRuleData* aRuleData);
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

protected:
  virtual ~ImportantRule();

  
  
  
  Declaration* mDeclaration;

  friend class StyleRule;
};

class StyleRule MOZ_FINAL : public Rule
{
 public:
  StyleRule(nsCSSSelectorList* aSelector,
            Declaration *aDeclaration);
private:
  
  StyleRule(const StyleRule& aCopy);
  
  StyleRule(StyleRule& aCopy,
            Declaration *aDeclaration);
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_CSS_STYLE_RULE_IMPL_CID)

  NS_DECL_ISUPPORTS_INHERITED

  
  nsCSSSelectorList* Selector() { return mSelector; }

  uint32_t GetLineNumber() const { return mLineNumber; }
  void SetLineNumber(uint32_t aLineNumber) { mLineNumber = aLineNumber; }

  Declaration* GetDeclaration() const { return mDeclaration; }

  








  already_AddRefed<StyleRule>
  DeclarationChanged(Declaration* aDecl, bool aHandleContainer);

  nsIStyleRule* GetImportantRule() const { return mImportantRule; }

  



  void RuleMatched();

  
  void GetCssText(nsAString& aCssText);
  void SetCssText(const nsAString& aCssText);
  void GetSelectorText(nsAString& aSelectorText);
  void SetSelectorText(const nsAString& aSelectorText);

  virtual int32_t GetType() const;

  virtual already_AddRefed<Rule> Clone() const;

  virtual nsIDOMCSSRule* GetDOMRule();

  
  virtual void MapRuleInfoInto(nsRuleData* aRuleData);

#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const;
#endif

  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

private:
  ~StyleRule();

private:
  nsCSSSelectorList*      mSelector; 
  Declaration*            mDeclaration;
  ImportantRule*          mImportantRule; 
  DOMCSSStyleRule*        mDOMRule;
  
  uint32_t                mLineNumber : 31;
  uint32_t                mWasMatched : 1;

private:
  StyleRule& operator=(const StyleRule& aCopy) MOZ_DELETE;
};

} 
} 

NS_DEFINE_STATIC_IID_ACCESSOR(mozilla::css::StyleRule, NS_CSS_STYLE_RULE_IMPL_CID)

#endif 
