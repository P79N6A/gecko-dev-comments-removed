










#include "mozilla/css/StyleRule.h"

#include "mozilla/CSSStyleSheet.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/css/GroupRule.h"
#include "mozilla/css/Declaration.h"
#include "nsIDocument.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "nsStyleUtil.h"
#include "nsICSSStyleRuleDOMWrapper.h"
#include "nsDOMCSSDeclaration.h"
#include "nsNameSpaceManager.h"
#include "nsXMLNameSpaceMap.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSPseudoClasses.h"
#include "nsCSSAnonBoxes.h"
#include "nsTArray.h"
#include "nsDOMClassInfoID.h"
#include "nsContentUtils.h"
#include "nsError.h"
#include "mozAutoDocUpdate.h"

class nsIDOMCSSStyleDeclaration;
class nsIDOMCSSStyleSheet;

using namespace mozilla;

#define NS_IF_CLONE(member_)                                                  \
  PR_BEGIN_MACRO                                                              \
    if (member_) {                                                            \
      result->member_ = member_->Clone();                                     \
      if (!result->member_) {                                                 \
        delete result;                                                        \
        return nullptr;                                                        \
      }                                                                       \
    }                                                                         \
  PR_END_MACRO

#define NS_IF_DELETE(ptr)                                                     \
  PR_BEGIN_MACRO                                                              \
    delete ptr;                                                               \
    ptr = nullptr;                                                             \
  PR_END_MACRO



nsAtomList::nsAtomList(nsIAtom* aAtom)
  : mAtom(aAtom),
    mNext(nullptr)
{
  MOZ_COUNT_CTOR(nsAtomList);
}

nsAtomList::nsAtomList(const nsString& aAtomValue)
  : mAtom(nullptr),
    mNext(nullptr)
{
  MOZ_COUNT_CTOR(nsAtomList);
  mAtom = do_GetAtom(aAtomValue);
}

nsAtomList*
nsAtomList::Clone(bool aDeep) const
{
  nsAtomList *result = new nsAtomList(mAtom);
  if (!result)
    return nullptr;

  if (aDeep)
    NS_CSS_CLONE_LIST_MEMBER(nsAtomList, this, mNext, result, (false));
  return result;
}

size_t
nsAtomList::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;
  const nsAtomList* a = this;
  while (a) {
    n += aMallocSizeOf(a);

    
    

    a = a->mNext;
  }
  return n;
}

nsAtomList::~nsAtomList(void)
{
  MOZ_COUNT_DTOR(nsAtomList);
  NS_CSS_DELETE_LIST_MEMBER(nsAtomList, this, mNext);
}

nsPseudoClassList::nsPseudoClassList(nsCSSPseudoClasses::Type aType)
  : mType(aType),
    mNext(nullptr)
{
  NS_ASSERTION(!nsCSSPseudoClasses::HasStringArg(aType) &&
               !nsCSSPseudoClasses::HasNthPairArg(aType),
               "unexpected pseudo-class");
  MOZ_COUNT_CTOR(nsPseudoClassList);
  u.mMemory = nullptr;
}

nsPseudoClassList::nsPseudoClassList(nsCSSPseudoClasses::Type aType,
                                     const char16_t* aString)
  : mType(aType),
    mNext(nullptr)
{
  NS_ASSERTION(nsCSSPseudoClasses::HasStringArg(aType),
               "unexpected pseudo-class");
  NS_ASSERTION(aString, "string expected");
  MOZ_COUNT_CTOR(nsPseudoClassList);
  u.mString = NS_strdup(aString);
}

nsPseudoClassList::nsPseudoClassList(nsCSSPseudoClasses::Type aType,
                                     const int32_t* aIntPair)
  : mType(aType),
    mNext(nullptr)
{
  NS_ASSERTION(nsCSSPseudoClasses::HasNthPairArg(aType),
               "unexpected pseudo-class");
  NS_ASSERTION(aIntPair, "integer pair expected");
  MOZ_COUNT_CTOR(nsPseudoClassList);
  u.mNumbers =
    static_cast<int32_t*>(nsMemory::Clone(aIntPair, sizeof(int32_t) * 2));
}


nsPseudoClassList::nsPseudoClassList(nsCSSPseudoClasses::Type aType,
                                     nsCSSSelectorList* aSelectorList)
  : mType(aType),
    mNext(nullptr)
{
  NS_ASSERTION(nsCSSPseudoClasses::HasSelectorListArg(aType),
               "unexpected pseudo-class");
  NS_ASSERTION(aSelectorList, "selector list expected");
  MOZ_COUNT_CTOR(nsPseudoClassList);
  u.mSelectors = aSelectorList;
}

nsPseudoClassList*
nsPseudoClassList::Clone(bool aDeep) const
{
  nsPseudoClassList *result;
  if (!u.mMemory) {
    result = new nsPseudoClassList(mType);
  } else if (nsCSSPseudoClasses::HasStringArg(mType)) {
    result = new nsPseudoClassList(mType, u.mString);
  } else if (nsCSSPseudoClasses::HasNthPairArg(mType)) {
    result = new nsPseudoClassList(mType, u.mNumbers);
  } else {
    NS_ASSERTION(nsCSSPseudoClasses::HasSelectorListArg(mType),
                 "unexpected pseudo-class");
    
    result = new nsPseudoClassList(mType, u.mSelectors->Clone());
  }

  if (aDeep)
    NS_CSS_CLONE_LIST_MEMBER(nsPseudoClassList, this, mNext, result,
                             (false));

  return result;
}

size_t
nsPseudoClassList::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;
  const nsPseudoClassList* p = this;
  while (p) {
    n += aMallocSizeOf(p);
    if (!p->u.mMemory) {
      

    } else if (nsCSSPseudoClasses::HasStringArg(p->mType)) {
      n += aMallocSizeOf(p->u.mString);

    } else if (nsCSSPseudoClasses::HasNthPairArg(p->mType)) {
      n += aMallocSizeOf(p->u.mNumbers);

    } else {
      NS_ASSERTION(nsCSSPseudoClasses::HasSelectorListArg(p->mType),
                   "unexpected pseudo-class");
      n += p->u.mSelectors->SizeOfIncludingThis(aMallocSizeOf);
    }
    p = p->mNext;
  }
  return n;
}

nsPseudoClassList::~nsPseudoClassList(void)
{
  MOZ_COUNT_DTOR(nsPseudoClassList);
  if (nsCSSPseudoClasses::HasSelectorListArg(mType)) {
    delete u.mSelectors;
  } else if (u.mMemory) {
    NS_Free(u.mMemory);
  }
  NS_CSS_DELETE_LIST_MEMBER(nsPseudoClassList, this, mNext);
}

nsAttrSelector::nsAttrSelector(int32_t aNameSpace, const nsString& aAttr)
  : mValue(),
    mNext(nullptr),
    mLowercaseAttr(nullptr),
    mCasedAttr(nullptr),
    mNameSpace(aNameSpace),
    mFunction(NS_ATTR_FUNC_SET),
    mCaseSensitive(1)
{
  MOZ_COUNT_CTOR(nsAttrSelector);

  nsAutoString lowercase;
  nsContentUtils::ASCIIToLower(aAttr, lowercase);
  
  mCasedAttr = do_GetAtom(aAttr);
  mLowercaseAttr = do_GetAtom(lowercase);
}

nsAttrSelector::nsAttrSelector(int32_t aNameSpace, const nsString& aAttr, uint8_t aFunction, 
                               const nsString& aValue, bool aCaseSensitive)
  : mValue(aValue),
    mNext(nullptr),
    mLowercaseAttr(nullptr),
    mCasedAttr(nullptr),
    mNameSpace(aNameSpace),
    mFunction(aFunction),
    mCaseSensitive(aCaseSensitive)
{
  MOZ_COUNT_CTOR(nsAttrSelector);

  nsAutoString lowercase;
  nsContentUtils::ASCIIToLower(aAttr, lowercase);
  
  mCasedAttr = do_GetAtom(aAttr);
  mLowercaseAttr = do_GetAtom(lowercase);
}

nsAttrSelector::nsAttrSelector(int32_t aNameSpace,  nsIAtom* aLowercaseAttr,
                               nsIAtom* aCasedAttr, uint8_t aFunction, 
                               const nsString& aValue, bool aCaseSensitive)
  : mValue(aValue),
    mNext(nullptr),
    mLowercaseAttr(aLowercaseAttr),
    mCasedAttr(aCasedAttr),
    mNameSpace(aNameSpace),
    mFunction(aFunction),
    mCaseSensitive(aCaseSensitive)
{
  MOZ_COUNT_CTOR(nsAttrSelector);
}

nsAttrSelector*
nsAttrSelector::Clone(bool aDeep) const
{
  nsAttrSelector *result =
    new nsAttrSelector(mNameSpace, mLowercaseAttr, mCasedAttr, 
                       mFunction, mValue, mCaseSensitive);

  if (aDeep)
    NS_CSS_CLONE_LIST_MEMBER(nsAttrSelector, this, mNext, result, (false));

  return result;
}

nsAttrSelector::~nsAttrSelector(void)
{
  MOZ_COUNT_DTOR(nsAttrSelector);

  NS_CSS_DELETE_LIST_MEMBER(nsAttrSelector, this, mNext);
}



nsCSSSelector::nsCSSSelector(void)
  : mLowercaseTag(nullptr),
    mCasedTag(nullptr),
    mIDList(nullptr),
    mClassList(nullptr),
    mPseudoClassList(nullptr),
    mAttrList(nullptr),
    mNegations(nullptr),
    mNext(nullptr),
    mNameSpace(kNameSpaceID_Unknown),
    mOperator(0),
    mPseudoType(nsCSSPseudoElements::ePseudo_NotPseudoElement)
{
  MOZ_COUNT_CTOR(nsCSSSelector);
  static_assert(nsCSSPseudoElements::ePseudo_MAX < INT16_MAX,
                "nsCSSPseudoElements::Type values overflow mPseudoType");
}

nsCSSSelector*
nsCSSSelector::Clone(bool aDeepNext, bool aDeepNegations) const
{
  nsCSSSelector *result = new nsCSSSelector();
  if (!result)
    return nullptr;

  result->mNameSpace = mNameSpace;
  result->mLowercaseTag = mLowercaseTag;
  result->mCasedTag = mCasedTag;
  result->mOperator = mOperator;
  result->mPseudoType = mPseudoType;
  
  NS_IF_CLONE(mIDList);
  NS_IF_CLONE(mClassList);
  NS_IF_CLONE(mPseudoClassList);
  NS_IF_CLONE(mAttrList);

  
  
  NS_ASSERTION(!mNegations || !mNegations->mNext,
               "mNegations can't have non-null mNext");
  if (aDeepNegations) {
    NS_CSS_CLONE_LIST_MEMBER(nsCSSSelector, this, mNegations, result,
                             (true, false));
  }

  if (aDeepNext) {
    NS_CSS_CLONE_LIST_MEMBER(nsCSSSelector, this, mNext, result,
                             (false, true));
  }

  return result;
}

nsCSSSelector::~nsCSSSelector(void)  
{
  MOZ_COUNT_DTOR(nsCSSSelector);
  Reset();
  
  
  NS_CSS_DELETE_LIST_MEMBER(nsCSSSelector, this, mNext);
}

void nsCSSSelector::Reset(void)
{
  mNameSpace = kNameSpaceID_Unknown;
  mLowercaseTag = nullptr;
  mCasedTag = nullptr;
  NS_IF_DELETE(mIDList);
  NS_IF_DELETE(mClassList);
  NS_IF_DELETE(mPseudoClassList);
  NS_IF_DELETE(mAttrList);
  
  
  NS_ASSERTION(!mNegations || !mNegations->mNext,
               "mNegations can't have non-null mNext");
  NS_CSS_DELETE_LIST_MEMBER(nsCSSSelector, this, mNegations);
  mOperator = char16_t(0);
}

void nsCSSSelector::SetNameSpace(int32_t aNameSpace)
{
  mNameSpace = aNameSpace;
}

void nsCSSSelector::SetTag(const nsString& aTag)
{
  if (aTag.IsEmpty()) {
    mLowercaseTag = mCasedTag =  nullptr;
    return;
  }

  mCasedTag = do_GetAtom(aTag);
 
  nsAutoString lowercase;
  nsContentUtils::ASCIIToLower(aTag, lowercase);
  mLowercaseTag = do_GetAtom(lowercase);
}

void nsCSSSelector::AddID(const nsString& aID)
{
  if (!aID.IsEmpty()) {
    nsAtomList** list = &mIDList;
    while (nullptr != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAtomList(aID);
  }
}

void nsCSSSelector::AddClass(const nsString& aClass)
{
  if (!aClass.IsEmpty()) {
    nsAtomList** list = &mClassList;
    while (nullptr != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAtomList(aClass);
  }
}

void nsCSSSelector::AddPseudoClass(nsCSSPseudoClasses::Type aType)
{
  AddPseudoClassInternal(new nsPseudoClassList(aType));
}

void nsCSSSelector::AddPseudoClass(nsCSSPseudoClasses::Type aType,
                                   const char16_t* aString)
{
  AddPseudoClassInternal(new nsPseudoClassList(aType, aString));
}

void nsCSSSelector::AddPseudoClass(nsCSSPseudoClasses::Type aType,
                                   const int32_t* aIntPair)
{
  AddPseudoClassInternal(new nsPseudoClassList(aType, aIntPair));
}

void nsCSSSelector::AddPseudoClass(nsCSSPseudoClasses::Type aType,
                                   nsCSSSelectorList* aSelectorList)
{
  
  AddPseudoClassInternal(new nsPseudoClassList(aType, aSelectorList));
}

void nsCSSSelector::AddPseudoClassInternal(nsPseudoClassList *aPseudoClass)
{
  nsPseudoClassList** list = &mPseudoClassList;
  while (nullptr != *list) {
    list = &((*list)->mNext);
  }
  *list = aPseudoClass;
}

void nsCSSSelector::AddAttribute(int32_t aNameSpace, const nsString& aAttr)
{
  if (!aAttr.IsEmpty()) {
    nsAttrSelector** list = &mAttrList;
    while (nullptr != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAttrSelector(aNameSpace, aAttr);
  }
}

void nsCSSSelector::AddAttribute(int32_t aNameSpace, const nsString& aAttr, uint8_t aFunc, 
                                 const nsString& aValue, bool aCaseSensitive)
{
  if (!aAttr.IsEmpty()) {
    nsAttrSelector** list = &mAttrList;
    while (nullptr != *list) {
      list = &((*list)->mNext);
    }
    *list = new nsAttrSelector(aNameSpace, aAttr, aFunc, aValue, aCaseSensitive);
  }
}

void nsCSSSelector::SetOperator(char16_t aOperator)
{
  mOperator = aOperator;
}

int32_t nsCSSSelector::CalcWeightWithoutNegations() const
{
  int32_t weight = 0;

#ifdef MOZ_XUL
  MOZ_ASSERT(!(IsPseudoElement() &&
               PseudoType() != nsCSSPseudoElements::ePseudo_XULTree &&
               mClassList),
             "If non-XUL-tree pseudo-elements can have class selectors "
             "after them, specificity calculation must be updated");
#else
  MOZ_ASSERT(!(IsPseudoElement() && mClassList),
             "If pseudo-elements can have class selectors "
             "after them, specificity calculation must be updated");
#endif
  MOZ_ASSERT(!(IsPseudoElement() && (mIDList || mAttrList)),
             "If pseudo-elements can have id or attribute selectors "
             "after them, specificity calculation must be updated");

  if (nullptr != mCasedTag) {
    weight += 0x000001;
  }
  nsAtomList* list = mIDList;
  while (nullptr != list) {
    weight += 0x010000;
    list = list->mNext;
  }
  list = mClassList;
#ifdef MOZ_XUL
  
  
  if (PseudoType() == nsCSSPseudoElements::ePseudo_XULTree) {
    list = nullptr;
  }
#endif
  while (nullptr != list) {
    weight += 0x000100;
    list = list->mNext;
  }
  
  
  
  
  
  nsPseudoClassList *plist = mPseudoClassList;
  while (nullptr != plist) {
    weight += 0x000100;
    plist = plist->mNext;
  }
  nsAttrSelector* attr = mAttrList;
  while (nullptr != attr) {
    weight += 0x000100;
    attr = attr->mNext;
  }
  return weight;
}

int32_t nsCSSSelector::CalcWeight() const
{
  
  int32_t weight = 0;
  for (const nsCSSSelector *n = this; n; n = n->mNegations) {
    weight += n->CalcWeightWithoutNegations();
  }
  return weight;
}





void
nsCSSSelector::ToString(nsAString& aString, CSSStyleSheet* aSheet,
                        bool aAppend) const
{
  if (!aAppend)
   aString.Truncate();

  
  
  nsAutoTArray<const nsCSSSelector*, 8> stack;
  for (const nsCSSSelector *s = this; s; s = s->mNext) {
    stack.AppendElement(s);
  }

  while (!stack.IsEmpty()) {
    uint32_t index = stack.Length() - 1;
    const nsCSSSelector *s = stack.ElementAt(index);
    stack.RemoveElementAt(index);

    s->AppendToStringWithoutCombinators(aString, aSheet);

    
    if (!stack.IsEmpty()) {
      const nsCSSSelector *next = stack.ElementAt(index - 1);
      char16_t oper = s->mOperator;
      if (next->IsPseudoElement()) {
        NS_ASSERTION(oper == char16_t(':'),
                     "improperly chained pseudo element");
      } else {
        NS_ASSERTION(oper != char16_t(0),
                     "compound selector without combinator");

        aString.Append(char16_t(' '));
        if (oper != char16_t(' ')) {
          aString.Append(oper);
          aString.Append(char16_t(' '));
        }
      }
    }
  }
}

void
nsCSSSelector::AppendToStringWithoutCombinators
                   (nsAString& aString, CSSStyleSheet* aSheet) const
{
  AppendToStringWithoutCombinatorsOrNegations(aString, aSheet, false);

  for (const nsCSSSelector* negation = mNegations; negation;
       negation = negation->mNegations) {
    aString.AppendLiteral(":not(");
    negation->AppendToStringWithoutCombinatorsOrNegations(aString, aSheet,
                                                          true);
    aString.Append(char16_t(')'));
  }
}

void
nsCSSSelector::AppendToStringWithoutCombinatorsOrNegations
                   (nsAString& aString, CSSStyleSheet* aSheet,
                   bool aIsNegated) const
{
  nsAutoString temp;
  bool isPseudoElement = IsPseudoElement();

  
  
  bool wroteNamespace = false;
  if (!isPseudoElement || !mNext) {
    
    nsXMLNameSpaceMap *sheetNS = aSheet ? aSheet->GetNameSpaceMap() : nullptr;

    
    
    
    
    if (!sheetNS) {
      NS_ASSERTION(mNameSpace == kNameSpaceID_Unknown ||
                   mNameSpace == kNameSpaceID_None,
                   "How did we get this namespace?");
      if (mNameSpace == kNameSpaceID_None) {
        aString.Append(char16_t('|'));
        wroteNamespace = true;
      }
    } else if (sheetNS->FindNameSpaceID(nullptr) == mNameSpace) {
      
      
      NS_ASSERTION(mNameSpace == kNameSpaceID_Unknown ||
                   CanBeNamespaced(aIsNegated),
                   "How did we end up with this namespace?");
    } else if (mNameSpace == kNameSpaceID_None) {
      NS_ASSERTION(CanBeNamespaced(aIsNegated),
                   "How did we end up with this namespace?");
      aString.Append(char16_t('|'));
      wroteNamespace = true;
    } else if (mNameSpace != kNameSpaceID_Unknown) {
      NS_ASSERTION(CanBeNamespaced(aIsNegated),
                   "How did we end up with this namespace?");
      nsIAtom *prefixAtom = sheetNS->FindPrefix(mNameSpace);
      NS_ASSERTION(prefixAtom, "how'd we get a non-default namespace "
                   "without a prefix?");
      nsStyleUtil::AppendEscapedCSSIdent(nsDependentAtomString(prefixAtom),
                                         aString);
      aString.Append(char16_t('|'));
      wroteNamespace = true;
    } else {
      
      
      
      
      
      if (CanBeNamespaced(aIsNegated)) {
        aString.AppendLiteral("*|");
        wroteNamespace = true;
      }
    }
  }
      
  if (!mLowercaseTag) {
    
    
    
    if (wroteNamespace ||
        (!mIDList && !mClassList && !mPseudoClassList && !mAttrList &&
         (aIsNegated || !mNegations))) {
      aString.Append(char16_t('*'));
    }
  } else {
    
    nsAutoString tag;
    (isPseudoElement ? mLowercaseTag : mCasedTag)->ToString(tag);
    if (isPseudoElement) {
      if (!mNext) {
        
        
        aString.Append(char16_t('*'));
      }
      
      
      
      aString.Append(char16_t(':'));
      
      
      
      
      aString.Append(tag);
    } else {
      nsStyleUtil::AppendEscapedCSSIdent(tag, aString);
    }
  }

  
  if (mIDList) {
    nsAtomList* list = mIDList;
    while (list != nullptr) {
      list->mAtom->ToString(temp);
      aString.Append(char16_t('#'));
      nsStyleUtil::AppendEscapedCSSIdent(temp, aString);
      list = list->mNext;
    }
  }

  
  if (mClassList) {
    if (isPseudoElement) {
#ifdef MOZ_XUL
      MOZ_ASSERT(nsCSSAnonBoxes::IsTreePseudoElement(mLowercaseTag),
                 "must be tree pseudo-element");

      aString.Append(char16_t('('));
      for (nsAtomList* list = mClassList; list; list = list->mNext) {
        nsStyleUtil::AppendEscapedCSSIdent(nsDependentAtomString(list->mAtom), aString);
        aString.Append(char16_t(','));
      }
      
      aString.Replace(aString.Length() - 1, 1, char16_t(')'));
#else
      NS_ERROR("Can't happen");
#endif
    } else {
      nsAtomList* list = mClassList;
      while (list != nullptr) {
        list->mAtom->ToString(temp);
        aString.Append(char16_t('.'));
        nsStyleUtil::AppendEscapedCSSIdent(temp, aString);
        list = list->mNext;
      }
    }
  }

  
  if (mAttrList) {
    nsAttrSelector* list = mAttrList;
    while (list != nullptr) {
      aString.Append(char16_t('['));
      
      if (list->mNameSpace == kNameSpaceID_Unknown) {
        aString.Append(char16_t('*'));
        aString.Append(char16_t('|'));
      } else if (list->mNameSpace != kNameSpaceID_None) {
        if (aSheet) {
          nsXMLNameSpaceMap *sheetNS = aSheet->GetNameSpaceMap();
          nsIAtom *prefixAtom = sheetNS->FindPrefix(list->mNameSpace);
          
          
          NS_ASSERTION(prefixAtom,
                       "How did we end up with a namespace if the prefix "
                       "is unknown?");
          nsAutoString prefix;
          prefixAtom->ToString(prefix);
          nsStyleUtil::AppendEscapedCSSIdent(prefix, aString);
          aString.Append(char16_t('|'));
        }
      }
      
      list->mCasedAttr->ToString(temp);
      nsStyleUtil::AppendEscapedCSSIdent(temp, aString);

      if (list->mFunction != NS_ATTR_FUNC_SET) {
        
        if (list->mFunction == NS_ATTR_FUNC_INCLUDES)
          aString.Append(char16_t('~'));
        else if (list->mFunction == NS_ATTR_FUNC_DASHMATCH)
          aString.Append(char16_t('|'));
        else if (list->mFunction == NS_ATTR_FUNC_BEGINSMATCH)
          aString.Append(char16_t('^'));
        else if (list->mFunction == NS_ATTR_FUNC_ENDSMATCH)
          aString.Append(char16_t('$'));
        else if (list->mFunction == NS_ATTR_FUNC_CONTAINSMATCH)
          aString.Append(char16_t('*'));

        aString.Append(char16_t('='));
      
        
        nsStyleUtil::AppendEscapedCSSString(list->mValue, aString);
      }

      aString.Append(char16_t(']'));
      
      list = list->mNext;
    }
  }

  
  for (nsPseudoClassList* list = mPseudoClassList; list; list = list->mNext) {
    nsCSSPseudoClasses::PseudoTypeToString(list->mType, temp);
    
    
    
    
    aString.Append(temp);
    if (list->u.mMemory) {
      aString.Append(char16_t('('));
      if (nsCSSPseudoClasses::HasStringArg(list->mType)) {
        nsStyleUtil::AppendEscapedCSSIdent(
          nsDependentString(list->u.mString), aString);
      } else if (nsCSSPseudoClasses::HasNthPairArg(list->mType)) {
        int32_t a = list->u.mNumbers[0],
                b = list->u.mNumbers[1];
        temp.Truncate();
        if (a != 0) {
          if (a == -1) {
            temp.Append(char16_t('-'));
          } else if (a != 1) {
            temp.AppendInt(a);
          }
          temp.Append(char16_t('n'));
        }
        if (b != 0 || a == 0) {
          if (b >= 0 && a != 0) 
            temp.Append(char16_t('+'));
          temp.AppendInt(b);
        }
        aString.Append(temp);
      } else {
        NS_ASSERTION(nsCSSPseudoClasses::HasSelectorListArg(list->mType),
                     "unexpected pseudo-class");
        nsString tmp;
        list->u.mSelectors->ToString(tmp, aSheet);
        aString.Append(tmp);
      }
      aString.Append(char16_t(')'));
    }
  }
}

bool
nsCSSSelector::CanBeNamespaced(bool aIsNegated) const
{
  return !aIsNegated ||
         (!mIDList && !mClassList && !mPseudoClassList && !mAttrList);
}

size_t
nsCSSSelector::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;
  const nsCSSSelector* s = this;
  while (s) {
    n += aMallocSizeOf(s);

    #define MEASURE(x)   n += x ? x->SizeOfIncludingThis(aMallocSizeOf) : 0;

    MEASURE(s->mIDList);
    MEASURE(s->mClassList);
    MEASURE(s->mPseudoClassList);
    MEASURE(s->mNegations);

    
    
    
    
    
    
    

    s = s->mNext;
  }
  return n;
}



nsCSSSelectorList::nsCSSSelectorList(void)
  : mSelectors(nullptr),
    mWeight(0),
    mNext(nullptr)
{
  MOZ_COUNT_CTOR(nsCSSSelectorList);
}

nsCSSSelectorList::~nsCSSSelectorList()
{
  MOZ_COUNT_DTOR(nsCSSSelectorList);
  delete mSelectors;
  NS_CSS_DELETE_LIST_MEMBER(nsCSSSelectorList, this, mNext);
}

nsCSSSelector*
nsCSSSelectorList::AddSelector(char16_t aOperator)
{
  nsCSSSelector* newSel = new nsCSSSelector();

  if (mSelectors) {
    NS_ASSERTION(aOperator != char16_t(0), "chaining without combinator");
    mSelectors->SetOperator(aOperator);
  } else {
    NS_ASSERTION(aOperator == char16_t(0), "combinator without chaining");
  }

  newSel->mNext = mSelectors;
  mSelectors = newSel;
  return newSel;
}

void
nsCSSSelectorList::RemoveRightmostSelector()
{
  nsCSSSelector* current = mSelectors;
  mSelectors = mSelectors->mNext;
  MOZ_ASSERT(mSelectors,
             "Rightmost selector has been removed, but now "
             "mSelectors is null");
  mSelectors->SetOperator(char16_t(0));

  
  current->mNext = nullptr;
  delete current;
}

void
nsCSSSelectorList::ToString(nsAString& aResult, CSSStyleSheet* aSheet)
{
  aResult.Truncate();
  nsCSSSelectorList *p = this;
  for (;;) {
    p->mSelectors->ToString(aResult, aSheet, true);
    p = p->mNext;
    if (!p)
      break;
    aResult.AppendLiteral(", ");
  }
}

nsCSSSelectorList*
nsCSSSelectorList::Clone(bool aDeep) const
{
  nsCSSSelectorList *result = new nsCSSSelectorList();
  result->mWeight = mWeight;
  NS_IF_CLONE(mSelectors);

  if (aDeep) {
    NS_CSS_CLONE_LIST_MEMBER(nsCSSSelectorList, this, mNext, result,
                             (false));
  }
  return result;
}

size_t
nsCSSSelectorList::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;
  const nsCSSSelectorList* s = this;
  while (s) {
    n += aMallocSizeOf(s);
    n += s->mSelectors ? s->mSelectors->SizeOfIncludingThis(aMallocSizeOf) : 0;
    s = s->mNext;
  }
  return n;
}



namespace mozilla {
namespace css {

ImportantRule::ImportantRule(Declaration* aDeclaration)
  : mDeclaration(aDeclaration)
{
}

ImportantRule::~ImportantRule()
{
}

NS_IMPL_ISUPPORTS(ImportantRule, nsIStyleRule)

 void
ImportantRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  mDeclaration->MapImportantRuleInfoInto(aRuleData);
}

#ifdef DEBUG
 void
ImportantRule::List(FILE* out, int32_t aIndent) const
{
  
  nsAutoCString str;
  for (int32_t index = aIndent; --index >= 0; ) {
    str.AppendLiteral("  ");
  }

  str.AppendLiteral("! important rule\n");
  fprintf_stderr(out, "%s", str.get());
}
#endif

} 
} 



namespace mozilla {
namespace css {
class DOMCSSStyleRule;
}
}

class DOMCSSDeclarationImpl : public nsDOMCSSDeclaration
{
protected:
  virtual ~DOMCSSDeclarationImpl(void);

public:
  explicit DOMCSSDeclarationImpl(css::StyleRule *aRule);

  NS_IMETHOD GetParentRule(nsIDOMCSSRule **aParent) override;
  void DropReference(void);
  virtual css::Declaration* GetCSSDeclaration(Operation aOperation) override;
  virtual nsresult SetCSSDeclaration(css::Declaration* aDecl) override;
  virtual void GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv) override;
  virtual nsIDocument* DocToUpdate() override;

  
  
  
  NS_DECL_ISUPPORTS_INHERITED

  virtual nsINode *GetParentObject() override
  {
    return mRule ? mRule->GetDocument() : nullptr;
  }

  friend class css::DOMCSSStyleRule;

protected:
  
  
  css::StyleRule *mRule;

  inline css::DOMCSSStyleRule* DomRule();

private:
  
  
  
  void* operator new(size_t size) CPP_THROW_NEW;
};

namespace mozilla {
namespace css {

class DOMCSSStyleRule : public nsICSSStyleRuleDOMWrapper
{
public:
  explicit DOMCSSStyleRule(StyleRule *aRule);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMCSSStyleRule)
  NS_DECL_NSIDOMCSSRULE
  NS_DECL_NSIDOMCSSSTYLERULE

  
  NS_IMETHOD GetCSSStyleRule(StyleRule **aResult) override;

  DOMCSSDeclarationImpl* DOMDeclaration() { return &mDOMDeclaration; }

  friend class ::DOMCSSDeclarationImpl;

protected:
  virtual ~DOMCSSStyleRule();

  DOMCSSDeclarationImpl mDOMDeclaration;

  StyleRule* Rule() {
    return mDOMDeclaration.mRule;
  }
};

} 
} 

DOMCSSDeclarationImpl::DOMCSSDeclarationImpl(css::StyleRule *aRule)
  : mRule(aRule)
{
  MOZ_COUNT_CTOR(DOMCSSDeclarationImpl);
}

DOMCSSDeclarationImpl::~DOMCSSDeclarationImpl(void)
{
  NS_ASSERTION(!mRule, "DropReference not called.");

  MOZ_COUNT_DTOR(DOMCSSDeclarationImpl);
}

inline css::DOMCSSStyleRule* DOMCSSDeclarationImpl::DomRule()
{
  return reinterpret_cast<css::DOMCSSStyleRule*>
                         (reinterpret_cast<char*>(this) -
           offsetof(css::DOMCSSStyleRule, mDOMDeclaration));
}

NS_IMPL_ADDREF_USING_AGGREGATOR(DOMCSSDeclarationImpl, DomRule())
NS_IMPL_RELEASE_USING_AGGREGATOR(DOMCSSDeclarationImpl, DomRule())

NS_INTERFACE_MAP_BEGIN(DOMCSSDeclarationImpl)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  
  
  if (aIID.Equals(NS_GET_IID(nsCycleCollectionISupports)) ||
      aIID.Equals(NS_GET_IID(nsXPCOMCycleCollectionParticipant))) {
    return DomRule()->QueryInterface(aIID, aInstancePtr);
  }
  else
NS_IMPL_QUERY_TAIL_INHERITING(nsDOMCSSDeclaration)

void
DOMCSSDeclarationImpl::DropReference(void)
{
  mRule = nullptr;
}

css::Declaration*
DOMCSSDeclarationImpl::GetCSSDeclaration(Operation aOperation)
{
  if (mRule) {
    return mRule->GetDeclaration();
  } else {
    return nullptr;
  }
}

void
DOMCSSDeclarationImpl::GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv)
{
  GetCSSParsingEnvironmentForRule(mRule, aCSSParseEnv);
}

NS_IMETHODIMP
DOMCSSDeclarationImpl::GetParentRule(nsIDOMCSSRule **aParent)
{
  NS_ENSURE_ARG_POINTER(aParent);

  if (!mRule) {
    *aParent = nullptr;
    return NS_OK;
  }

  NS_IF_ADDREF(*aParent = mRule->GetDOMRule());
  return NS_OK;
}

nsresult
DOMCSSDeclarationImpl::SetCSSDeclaration(css::Declaration* aDecl)
{
  NS_PRECONDITION(mRule,
         "can only be called when |GetCSSDeclaration| returned a declaration");

  nsCOMPtr<nsIDocument> owningDoc;
  nsCOMPtr<nsIStyleSheet> sheet = mRule->GetStyleSheet();
  if (sheet) {
    owningDoc = sheet->GetOwningDocument();
  }

  mozAutoDocUpdate updateBatch(owningDoc, UPDATE_STYLE, true);

  nsRefPtr<css::StyleRule> oldRule = mRule;
  mRule = oldRule->DeclarationChanged(aDecl, true).take();
  if (!mRule)
    return NS_ERROR_OUT_OF_MEMORY;
  nsrefcnt cnt = mRule->Release();
  if (cnt == 0) {
    NS_NOTREACHED("container didn't take ownership");
    mRule = nullptr;
    return NS_ERROR_UNEXPECTED;
  }

  if (owningDoc) {
    owningDoc->StyleRuleChanged(sheet, oldRule, mRule);
  }
  return NS_OK;
}

nsIDocument*
DOMCSSDeclarationImpl::DocToUpdate()
{
  return nullptr;
}

namespace mozilla {
namespace css {

DOMCSSStyleRule::DOMCSSStyleRule(StyleRule* aRule)
  : mDOMDeclaration(aRule)
{
}

DOMCSSStyleRule::~DOMCSSStyleRule()
{
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMCSSStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsICSSStyleRuleDOMWrapper)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSStyleRule)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMCSSStyleRule)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMCSSStyleRule)

NS_IMPL_CYCLE_COLLECTION_CLASS(DOMCSSStyleRule)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(DOMCSSStyleRule)
  
  
  
  tmp->DOMDeclaration()->TraceWrapper(aCallbacks, aClosure);
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(DOMCSSStyleRule)
  
  
  
  tmp->DOMDeclaration()->ReleaseWrapper(static_cast<nsISupports*>(p));
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(DOMCSSStyleRule)
  
  
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMETHODIMP
DOMCSSStyleRule::GetType(uint16_t* aType)
{
  *aType = nsIDOMCSSRule::STYLE_RULE;
  
  return NS_OK;
}

NS_IMETHODIMP
DOMCSSStyleRule::GetCssText(nsAString& aCssText)
{
  if (!Rule()) {
    aCssText.Truncate();
  } else {
    Rule()->GetCssText(aCssText);
  }
  return NS_OK;
}

NS_IMETHODIMP
DOMCSSStyleRule::SetCssText(const nsAString& aCssText)
{
  if (Rule()) {
    Rule()->SetCssText(aCssText);
  }
  return NS_OK;
}

NS_IMETHODIMP
DOMCSSStyleRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  if (!Rule()) {
    *aSheet = nullptr;
    return NS_OK;
  }
  return Rule()->GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
DOMCSSStyleRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  if (!Rule()) {
    *aParentRule = nullptr;
    return NS_OK;
  }
  return Rule()->GetParentRule(aParentRule);
}

css::Rule*
DOMCSSStyleRule::GetCSSRule()
{
  return Rule();
}

NS_IMETHODIMP
DOMCSSStyleRule::GetSelectorText(nsAString& aSelectorText)
{
  if (!Rule()) {
    aSelectorText.Truncate();
  } else {
    Rule()->GetSelectorText(aSelectorText);
  }
  return NS_OK;
}

NS_IMETHODIMP
DOMCSSStyleRule::SetSelectorText(const nsAString& aSelectorText)
{
  if (Rule()) {
    Rule()->SetSelectorText(aSelectorText);
  }
  return NS_OK;
}

NS_IMETHODIMP
DOMCSSStyleRule::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  *aStyle = &mDOMDeclaration;
  NS_ADDREF(*aStyle);
  return NS_OK;
}

NS_IMETHODIMP
DOMCSSStyleRule::GetCSSStyleRule(StyleRule **aResult)
{
  *aResult = Rule();
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

} 
} 



namespace mozilla {
namespace css {

StyleRule::StyleRule(nsCSSSelectorList* aSelector,
                     Declaration* aDeclaration,
                     uint32_t aLineNumber,
                     uint32_t aColumnNumber)
  : Rule(aLineNumber, aColumnNumber),
    mSelector(aSelector),
    mDeclaration(aDeclaration)
{
  NS_PRECONDITION(aDeclaration, "must have a declaration");
}


StyleRule::StyleRule(const StyleRule& aCopy)
  : Rule(aCopy),
    mSelector(aCopy.mSelector ? aCopy.mSelector->Clone() : nullptr),
    mDeclaration(new Declaration(*aCopy.mDeclaration))
{
  
}


StyleRule::StyleRule(StyleRule& aCopy,
                     Declaration* aDeclaration)
  : Rule(aCopy),
    mSelector(aCopy.mSelector),
    mDeclaration(aDeclaration),
    mDOMRule(aCopy.mDOMRule.forget())
{
  
  

  
  aCopy.mSelector = nullptr;

  
  
  
  if (mDeclaration == aCopy.mDeclaration) {
    
    mDeclaration->AssertMutable();
    aCopy.mDeclaration = nullptr;
  }
}

StyleRule::~StyleRule()
{
  delete mSelector;
  delete mDeclaration;
  if (mDOMRule) {
    mDOMRule->DOMDeclaration()->DropReference();
  }
}


NS_INTERFACE_MAP_BEGIN(StyleRule)
  if (aIID.Equals(NS_GET_IID(mozilla::css::StyleRule))) {
    *aInstancePtr = this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStyleRule)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(StyleRule)
NS_IMPL_RELEASE(StyleRule)

void
StyleRule::RuleMatched()
{
  if (!mWasMatched) {
    MOZ_ASSERT(!mImportantRule, "should not have important rule yet");

    mWasMatched = true;
    mDeclaration->SetImmutable();
    if (mDeclaration->HasImportantData()) {
      mImportantRule = new ImportantRule(mDeclaration);
    }
  }
}

 int32_t
StyleRule::GetType() const
{
  return Rule::STYLE_RULE;
}

 already_AddRefed<Rule>
StyleRule::Clone() const
{
  nsRefPtr<Rule> clone = new StyleRule(*this);
  return clone.forget();
}

 nsIDOMCSSRule*
StyleRule::GetDOMRule()
{
  if (!mDOMRule) {
    if (!GetStyleSheet()) {
      
      
      
      return nullptr;
    }
    mDOMRule = new DOMCSSStyleRule(this);
  }
  return mDOMRule;
}

 nsIDOMCSSRule*
StyleRule::GetExistingDOMRule()
{
  return mDOMRule;
}

 already_AddRefed<StyleRule>
StyleRule::DeclarationChanged(Declaration* aDecl,
                              bool aHandleContainer)
{
  nsRefPtr<StyleRule> clone = new StyleRule(*this, aDecl);

  if (aHandleContainer) {
    CSSStyleSheet* sheet = GetStyleSheet();
    if (mParentRule) {
      if (sheet) {
        sheet->ReplaceRuleInGroup(mParentRule, this, clone);
      } else {
        mParentRule->ReplaceStyleRule(this, clone);
      }
    } else if (sheet) {
      sheet->ReplaceStyleRule(this, clone);
    }
  }

  return clone.forget();
}

 void
StyleRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  MOZ_ASSERT(mWasMatched,
             "somebody forgot to call css::StyleRule::RuleMatched");
  mDeclaration->MapNormalRuleInfoInto(aRuleData);
}

#ifdef DEBUG
 void
StyleRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString str;
  
  for (int32_t index = aIndent; --index >= 0; ) {
    str.AppendLiteral("  ");
  }

  if (mSelector) {
    nsAutoString buffer;
    mSelector->ToString(buffer, GetStyleSheet());
    AppendUTF16toUTF8(buffer, str);
    str.Append(' ');
  }

  if (nullptr != mDeclaration) {
    nsAutoString buffer;
    str.AppendLiteral("{ ");
    mDeclaration->ToString(buffer);
    AppendUTF16toUTF8(buffer, str);
    str.Append('}');
    CSSStyleSheet* sheet = GetStyleSheet();
    if (sheet) {
      nsIURI* uri = sheet->GetSheetURI();
      if (uri) {
        nsAutoCString uristr;
        str.Append(" /* ");
        uri->GetSpec(uristr);
        str.Append(uristr);
        str.Append(':');
        str.AppendInt(mLineNumber);
        str.Append(" */");
      }
    }
  }
  else {
    str.AppendLiteral("{ null declaration }");
  }
  str.Append('\n');
  fprintf_stderr(out, "%s", str.get());
}
#endif

void
StyleRule::GetCssText(nsAString& aCssText)
{
  if (mSelector) {
    mSelector->ToString(aCssText, GetStyleSheet());
    aCssText.Append(char16_t(' '));
  }
  aCssText.Append(char16_t('{'));
  aCssText.Append(char16_t(' '));
  if (mDeclaration)
  {
    nsAutoString   tempString;
    mDeclaration->ToString( tempString );
    aCssText.Append( tempString );
  }
  aCssText.Append(char16_t(' '));
  aCssText.Append(char16_t('}'));
}

void
StyleRule::SetCssText(const nsAString& aCssText)
{
  
}

void
StyleRule::GetSelectorText(nsAString& aSelectorText)
{
  if (mSelector)
    mSelector->ToString(aSelectorText, GetStyleSheet());
  else
    aSelectorText.Truncate();
}

void
StyleRule::SetSelectorText(const nsAString& aSelectorText)
{
  
  
  
}

 size_t
StyleRule::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);
  n += mSelector ? mSelector->SizeOfIncludingThis(aMallocSizeOf) : 0;
  n += mDeclaration ? mDeclaration->SizeOfIncludingThis(aMallocSizeOf) : 0;

  
  
  
  

  return n;
}


} 
} 
