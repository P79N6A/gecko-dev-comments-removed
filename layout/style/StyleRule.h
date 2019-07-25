











































#ifndef mozilla_css_StyleRule_h__
#define mozilla_css_StyleRule_h__


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

  
  nsAtomList* Clone() const { return Clone(PR_TRUE); }

  nsCOMPtr<nsIAtom> mAtom;
  nsAtomList*       mNext;
private: 
  nsAtomList* Clone(PRBool aDeep) const;

  
  nsAtomList(const nsAtomList& aCopy);
  nsAtomList& operator=(const nsAtomList& aCopy); 
};

struct nsPseudoClassList {
public:
  nsPseudoClassList(nsCSSPseudoClasses::Type aType);
  nsPseudoClassList(nsCSSPseudoClasses::Type aType, const PRUnichar *aString);
  nsPseudoClassList(nsCSSPseudoClasses::Type aType, const PRInt32 *aIntPair);
  nsPseudoClassList(nsCSSPseudoClasses::Type aType,
                    nsCSSSelectorList *aSelectorList );
  ~nsPseudoClassList(void);

  
  nsPseudoClassList* Clone() const { return Clone(PR_TRUE); }

  union {
    
    
    
    
    
    
    
    
    
    void*           mMemory; 
    PRUnichar*      mString;
    PRInt32*        mNumbers;
    nsCSSSelectorList* mSelectors;
  } u;
  nsCSSPseudoClasses::Type mType;
  nsPseudoClassList* mNext;
private: 
  nsPseudoClassList* Clone(PRBool aDeep) const;

  
  nsPseudoClassList(const nsPseudoClassList& aCopy);
  nsPseudoClassList& operator=(const nsPseudoClassList& aCopy); 
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
  nsAttrSelector(PRInt32 aNameSpace, const nsString& aAttr);
  nsAttrSelector(PRInt32 aNameSpace, const nsString& aAttr, PRUint8 aFunction, 
                 const nsString& aValue, PRBool aCaseSensitive);
  nsAttrSelector(PRInt32 aNameSpace, nsIAtom* aLowercaseAttr, 
                 nsIAtom* aCasedAttr, PRUint8 aFunction, 
                 const nsString& aValue, PRBool aCaseSensitive);
  ~nsAttrSelector(void);

  
  nsAttrSelector* Clone() const { return Clone(PR_TRUE); }

  nsString        mValue;
  nsAttrSelector* mNext;
  nsCOMPtr<nsIAtom> mLowercaseAttr;
  nsCOMPtr<nsIAtom> mCasedAttr;
  PRInt32         mNameSpace;
  PRUint8         mFunction;
  PRPackedBool    mCaseSensitive; 
                                  
private: 
  nsAttrSelector* Clone(PRBool aDeep) const;

  
  nsAttrSelector(const nsAttrSelector& aCopy);
  nsAttrSelector& operator=(const nsAttrSelector& aCopy); 
};

struct nsCSSSelector {
public:
  nsCSSSelector(void);
  ~nsCSSSelector(void);

  
  nsCSSSelector* Clone() const { return Clone(PR_TRUE, PR_TRUE); }

  void Reset(void);
  void SetNameSpace(PRInt32 aNameSpace);
  void SetTag(const nsString& aTag);
  void AddID(const nsString& aID);
  void AddClass(const nsString& aClass);
  void AddPseudoClass(nsCSSPseudoClasses::Type aType);
  void AddPseudoClass(nsCSSPseudoClasses::Type aType, const PRUnichar* aString);
  void AddPseudoClass(nsCSSPseudoClasses::Type aType, const PRInt32* aIntPair);
  
  void AddPseudoClass(nsCSSPseudoClasses::Type aType,
                      nsCSSSelectorList* aSelectorList);
  void AddAttribute(PRInt32 aNameSpace, const nsString& aAttr);
  void AddAttribute(PRInt32 aNameSpace, const nsString& aAttr, PRUint8 aFunc, 
                    const nsString& aValue, PRBool aCaseSensitive);
  void SetOperator(PRUnichar aOperator);

  inline PRBool HasTagSelector() const {
    return !!mCasedTag;
  }

  inline PRBool IsPseudoElement() const {
    return mLowercaseTag && !mCasedTag;
  }

  
  PRInt32 CalcWeight() const;

  void ToString(nsAString& aString, nsCSSStyleSheet* aSheet,
                PRBool aAppend = PR_FALSE) const;

private:
  void AddPseudoClassInternal(nsPseudoClassList *aPseudoClass);
  nsCSSSelector* Clone(PRBool aDeepNext, PRBool aDeepNegations) const;

  void AppendToStringWithoutCombinators(nsAString& aString,
                                        nsCSSStyleSheet* aSheet) const;
  void AppendToStringWithoutCombinatorsOrNegations(nsAString& aString,
                                                   nsCSSStyleSheet* aSheet,
                                                   PRBool aIsNegated)
                                                        const;
  
  
  
  PRBool CanBeNamespaced(PRBool aIsNegated) const;
  
  
  PRInt32 CalcWeightWithoutNegations() const;

public:
  
  nsCSSPseudoElements::Type PseudoType() const {
    return static_cast<nsCSSPseudoElements::Type>(mPseudoType);
  }
  void SetPseudoType(nsCSSPseudoElements::Type aType) {
    NS_ASSERTION(aType > PR_INT16_MIN && aType < PR_INT16_MAX, "Out of bounds");
    mPseudoType = static_cast<PRInt16>(aType);
  }

  
  
  
  
  nsCOMPtr<nsIAtom> mLowercaseTag;
  nsCOMPtr<nsIAtom> mCasedTag;
  nsAtomList*     mIDList;
  nsAtomList*     mClassList;
  nsPseudoClassList* mPseudoClassList; 
                                       
  nsAttrSelector* mAttrList;
  nsCSSSelector*  mNegations;
  nsCSSSelector*  mNext;
  PRInt32         mNameSpace;
  PRUnichar       mOperator;
private:
  
  PRInt16        mPseudoType;
  
  nsCSSSelector(const nsCSSSelector& aCopy);
  nsCSSSelector& operator=(const nsCSSSelector& aCopy); 
};







struct nsCSSSelectorList {
  nsCSSSelectorList(void);
  ~nsCSSSelectorList(void);

  








  nsCSSSelector* AddSelector(PRUnichar aOperator);

  


  void ToString(nsAString& aResult, nsCSSStyleSheet* aSheet);

  


  nsCSSSelectorList* Clone() const { return Clone(PR_TRUE); }

  nsCSSSelector*     mSelectors;
  PRInt32            mWeight;
  nsCSSSelectorList* mNext;
private: 
  nsCSSSelectorList* Clone(PRBool aDeep) const;

  
  nsCSSSelectorList(const nsCSSSelectorList& aCopy);
  nsCSSSelectorList& operator=(const nsCSSSelectorList& aCopy); 
};


#define NS_CSS_STYLE_RULE_IMPL_CID \
{ 0x464bab7a, 0x2fce, 0x4f30, \
  { 0xab, 0x44, 0xb7, 0xa5, 0xf3, 0xaa, 0xe5, 0x7d } }

namespace mozilla {
namespace css {

class Declaration;
class ImportantRule;
class DOMCSSStyleRule;

class NS_FINAL_CLASS StyleRule : public Rule
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

  PRUint32 GetLineNumber() const { return mLineNumber; }
  void SetLineNumber(PRUint32 aLineNumber) { mLineNumber = aLineNumber; }

  Declaration* GetDeclaration() const { return mDeclaration; }

  








  already_AddRefed<StyleRule>
  DeclarationChanged(Declaration* aDecl, PRBool aHandleContainer);

  nsIStyleRule* GetImportantRule();

  



  void RuleMatched();

  
  void GetCssText(nsAString& aCssText);
  void SetCssText(const nsAString& aCssText);
  void GetSelectorText(nsAString& aSelectorText);
  void SetSelectorText(const nsAString& aSelectorText);

  virtual PRInt32 GetType() const;

  virtual already_AddRefed<Rule> Clone() const;

  nsIDOMCSSRule* GetDOMRuleWeak(nsresult* aResult);

  
  virtual void MapRuleInfoInto(nsRuleData* aRuleData);

#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

private:
  
  StyleRule& operator=(const StyleRule& aCopy);

private:
  ~StyleRule();

private:
  nsCSSSelectorList*      mSelector; 
  Declaration*            mDeclaration;
  ImportantRule*          mImportantRule; 
  DOMCSSStyleRule*        mDOMRule;
  
  PRUint32                mLineNumber : 31;
  PRUint32                mWasMatched : 1;
};

} 
} 

NS_DEFINE_STATIC_IID_ACCESSOR(mozilla::css::StyleRule, NS_CSS_STYLE_RULE_IMPL_CID)

#endif 
