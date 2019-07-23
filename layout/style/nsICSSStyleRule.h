











































#ifndef nsICSSStyleRule_h___
#define nsICSSStyleRule_h___


#include "nsICSSRule.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsCSSProps.h"
#include "nsCSSValue.h"
#include "nsIAtom.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSPseudoClasses.h"

class nsIAtom;
class nsCSSDeclaration;
class nsICSSStyleSheet;

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
  nsPseudoClassList(nsIAtom* aAtom, nsCSSPseudoClasses::Type aType);
  nsPseudoClassList(nsIAtom* aAtom, nsCSSPseudoClasses::Type aType,
                    const PRUnichar *aString);
  nsPseudoClassList(nsIAtom* aAtom, nsCSSPseudoClasses::Type aType,
                    const PRInt32 *aIntPair);
  ~nsPseudoClassList(void);

  
  nsPseudoClassList* Clone() const { return Clone(PR_TRUE); }

  nsCOMPtr<nsIAtom> mAtom;
  union {
    
    
    
    
    
    
    
    void*           mMemory; 
    PRUnichar*      mString;
    PRInt32*        mNumbers;
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
  void AddPseudoClass(nsIAtom* aPseudoClass, nsCSSPseudoClasses::Type aType);
  void AddPseudoClass(nsIAtom* aPseudoClass, nsCSSPseudoClasses::Type aType,
                      const PRUnichar* aString);
  void AddPseudoClass(nsIAtom* aPseudoClass, nsCSSPseudoClasses::Type aType,
                      const PRInt32* aIntPair);
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

  void ToString(nsAString& aString, nsICSSStyleSheet* aSheet,
                PRBool aAppend = PR_FALSE) const;

private:
  void AddPseudoClassInternal(nsPseudoClassList *aPseudoClass);
  nsCSSSelector* Clone(PRBool aDeepNext, PRBool aDeepNegations) const;

  void AppendToStringWithoutCombinators(nsAString& aString,
                                        nsICSSStyleSheet* aSheet) const;
  void AppendToStringWithoutCombinatorsOrNegations(nsAString& aString,
                                                   nsICSSStyleSheet* aSheet,
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

  






  void AddSelector(nsAutoPtr<nsCSSSelector>& aSelector);

  


  void ToString(nsAString& aResult, nsICSSStyleSheet* aSheet);

  


  nsCSSSelectorList* Clone() const { return Clone(PR_TRUE); }

  nsCSSSelector*     mSelectors;
  PRInt32            mWeight;
  nsCSSSelectorList* mNext;
private: 
  nsCSSSelectorList* Clone(PRBool aDeep) const;

  
  nsCSSSelectorList(const nsCSSSelectorList& aCopy);
  nsCSSSelectorList& operator=(const nsCSSSelectorList& aCopy); 
};


#define NS_ICSS_STYLE_RULE_IID     \
{ 0x3ffbd89e, 0x3c83, 0x4e9b, \
 { 0x9b, 0x1f, 0x42, 0x4c, 0x6c, 0xeb, 0xac, 0x1b } }

class nsICSSStyleRule : public nsICSSRule {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_STYLE_RULE_IID)

  
  virtual nsCSSSelectorList* Selector(void) = 0;

  virtual PRUint32 GetLineNumber(void) const = 0;
  virtual void SetLineNumber(PRUint32 aLineNumber) = 0;

  virtual nsCSSDeclaration* GetDeclaration(void) const = 0;

  








  virtual already_AddRefed<nsICSSStyleRule>
    DeclarationChanged(PRBool aHandleContainer) = 0;

  
  virtual nsresult GetCssText(nsAString& aCssText) = 0;
  virtual nsresult SetCssText(const nsAString& aCssText) = 0;
  virtual nsresult GetParentStyleSheet(nsICSSStyleSheet** aSheet) = 0;
  virtual nsresult GetParentRule(nsICSSGroupRule** aParentRule) = 0;
  virtual nsresult GetSelectorText(nsAString& aSelectorText) = 0;
  virtual nsresult SetSelectorText(const nsAString& aSelectorText) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSStyleRule, NS_ICSS_STYLE_RULE_IID)

nsresult
NS_NewCSSStyleRule(nsICSSStyleRule** aInstancePtrResult,
                   nsCSSSelectorList* aSelector,
                   nsCSSDeclaration* aDeclaration);

#endif 
