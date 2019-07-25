







































#include "nsCSSRules.h"
#include "nsCSSValue.h"
#include "nsICSSImportRule.h"
#include "nsICSSNameSpaceRule.h"

#include "nsString.h"
#include "nsIAtom.h"
#include "nsIURL.h"

#include "nsCSSRule.h"
#include "nsCSSProps.h"
#include "nsCSSStyleSheet.h"

#include "nsCOMPtr.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsIDOMCSSRule.h"
#include "nsIDOMCSSImportRule.h"
#include "nsIDOMCSSMediaRule.h"
#include "nsIDOMCSSMozDocumentRule.h"
#include "nsIDOMCSSCharsetRule.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIMediaList.h"
#include "nsIDOMMediaList.h"
#include "nsICSSRuleList.h"
#include "nsIDOMStyleSheet.h"
#include "nsIDocument.h"
#include "nsPresContext.h"

#include "nsContentUtils.h"
#include "nsStyleConsts.h"
#include "nsDOMError.h"
#include "nsStyleUtil.h"
#include "mozilla/css/Declaration.h"
#include "nsPrintfCString.h"

namespace css = mozilla::css;

#define IMPL_STYLE_RULE_INHERIT(_class, super) \
/* virtual */ already_AddRefed<nsIStyleSheet> _class::GetStyleSheet() const { return super::GetStyleSheet(); }  \
/* virtual */ void _class::SetStyleSheet(nsCSSStyleSheet* aSheet) { super::SetStyleSheet(aSheet); }  \
/* virtual */ void _class::SetParentRule(nsICSSGroupRule* aRule) { super::SetParentRule(aRule); }  \
nsIDOMCSSRule* _class::GetDOMRuleWeak(nsresult *aResult) { *aResult = NS_OK; return this; }  \
/* virtual */ void _class::MapRuleInfoInto(nsRuleData* aRuleData) { }

#define IMPL_STYLE_RULE_INHERIT2(_class, super) \
/* virtual */ already_AddRefed<nsIStyleSheet> _class::GetStyleSheet() const { return super::GetStyleSheet(); }  \
/* virtual */ void  _class::SetParentRule(nsICSSGroupRule* aRule) { super::SetParentRule(aRule); }  \
/* virtual */ void _class::MapRuleInfoInto(nsRuleData* aRuleData) { }




class CSSGroupRuleRuleListImpl : public nsICSSRuleList
{
public:
  CSSGroupRuleRuleListImpl(nsICSSGroupRule *aGroupRule);

  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMCSSRULELIST

  virtual nsIDOMCSSRule* GetItemAt(PRUint32 aIndex, nsresult* aResult);

  void DropReference() { mGroupRule = nsnull; }

protected:
  virtual ~CSSGroupRuleRuleListImpl(void);

private:
  nsICSSGroupRule* mGroupRule;
};

CSSGroupRuleRuleListImpl::CSSGroupRuleRuleListImpl(nsICSSGroupRule *aGroupRule)
{
  
  
  mGroupRule = aGroupRule;
}

CSSGroupRuleRuleListImpl::~CSSGroupRuleRuleListImpl()
{
}

DOMCI_DATA(CSSGroupRuleRuleList, CSSGroupRuleRuleListImpl)


NS_INTERFACE_MAP_BEGIN(CSSGroupRuleRuleListImpl)
  NS_INTERFACE_MAP_ENTRY(nsICSSRuleList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRuleList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSGroupRuleRuleList)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(CSSGroupRuleRuleListImpl)
NS_IMPL_RELEASE(CSSGroupRuleRuleListImpl)

NS_IMETHODIMP    
CSSGroupRuleRuleListImpl::GetLength(PRUint32* aLength)
{
  if (mGroupRule) {
    PRInt32 count;
    mGroupRule->StyleRuleCount(count);
    *aLength = (PRUint32)count;
  } else {
    *aLength = 0;
  }

  return NS_OK;
}

nsIDOMCSSRule*    
CSSGroupRuleRuleListImpl::GetItemAt(PRUint32 aIndex, nsresult* aResult)
{
  nsresult result = NS_OK;

  if (mGroupRule) {
    nsCOMPtr<nsICSSRule> rule;

    result = mGroupRule->GetStyleRuleAt(aIndex, *getter_AddRefs(rule));
    if (rule) {
      return rule->GetDOMRuleWeak(aResult);
    }
    if (result == NS_ERROR_ILLEGAL_VALUE) {
      result = NS_OK; 
    }
  }

  *aResult = result;

  return nsnull;
}

NS_IMETHODIMP    
CSSGroupRuleRuleListImpl::Item(PRUint32 aIndex, nsIDOMCSSRule** aReturn)
{
  nsresult rv;
  nsIDOMCSSRule* rule = GetItemAt(aIndex, &rv);
  if (!rule) {
    *aReturn = nsnull;
    return rv;
  }

  NS_ADDREF(*aReturn = rule);
  return NS_OK;
}




class NS_FINAL_CLASS CSSCharsetRuleImpl : public nsCSSRule,
                                          public nsICSSRule,
                                          public nsIDOMCSSCharsetRule
{
public:
  CSSCharsetRuleImpl(const nsAString& aEncoding);
  CSSCharsetRuleImpl(const CSSCharsetRuleImpl& aCopy);
private:
  ~CSSCharsetRuleImpl() {}
public:
  NS_DECL_ISUPPORTS

  DECL_STYLE_RULE_INHERIT

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  virtual PRInt32 GetType() const;
  virtual already_AddRefed<nsICSSRule> Clone() const;

  
  NS_DECL_NSIDOMCSSRULE
  
  
  NS_IMETHOD GetEncoding(nsAString& aEncoding);
  NS_IMETHOD SetEncoding(const nsAString& aEncoding);

protected:
  nsString  mEncoding;
};

CSSCharsetRuleImpl::CSSCharsetRuleImpl(const nsAString& aEncoding)
  : nsCSSRule(),
    mEncoding(aEncoding)
{
}

CSSCharsetRuleImpl::CSSCharsetRuleImpl(const CSSCharsetRuleImpl& aCopy)
  : nsCSSRule(aCopy),
    mEncoding(aCopy.mEncoding)
{
}

NS_IMPL_ADDREF(CSSCharsetRuleImpl)
NS_IMPL_RELEASE(CSSCharsetRuleImpl)

DOMCI_DATA(CSSCharsetRule, CSSCharsetRuleImpl)


NS_INTERFACE_MAP_BEGIN(CSSCharsetRuleImpl)
  NS_INTERFACE_MAP_ENTRY(nsICSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSCharsetRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICSSRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSCharsetRule)
NS_INTERFACE_MAP_END

IMPL_STYLE_RULE_INHERIT(CSSCharsetRuleImpl, nsCSSRule)

#ifdef DEBUG
 void
CSSCharsetRuleImpl::List(FILE* out, PRInt32 aIndent) const
{
  
  for (PRInt32 indent = aIndent; --indent >= 0; ) fputs("  ", out);

  fputs("@charset \"", out);
  fputs(NS_LossyConvertUTF16toASCII(mEncoding).get(), out);
  fputs("\"\n", out);
}
#endif

 PRInt32
CSSCharsetRuleImpl::GetType() const
{
  return nsICSSRule::CHARSET_RULE;
}

 already_AddRefed<nsICSSRule>
CSSCharsetRuleImpl::Clone() const
{
  nsCOMPtr<nsICSSRule> clone = new CSSCharsetRuleImpl(*this);
  return clone.forget();
}

NS_IMETHODIMP
CSSCharsetRuleImpl::GetEncoding(nsAString& aEncoding)
{
  aEncoding = mEncoding;
  return NS_OK;
}

NS_IMETHODIMP
CSSCharsetRuleImpl::SetEncoding(const nsAString& aEncoding)
{
  mEncoding = aEncoding;
  return NS_OK;
}


nsresult
NS_NewCSSCharsetRule(nsICSSRule** aInstancePtrResult, const nsAString& aEncoding)
{
  if (! aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }

  CSSCharsetRuleImpl* it = new CSSCharsetRuleImpl(aEncoding);

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult = it);
  return NS_OK;
}

NS_IMETHODIMP
CSSCharsetRuleImpl::GetType(PRUint16* aType)
{
  *aType = nsIDOMCSSRule::CHARSET_RULE;
  return NS_OK;
}

NS_IMETHODIMP
CSSCharsetRuleImpl::GetCssText(nsAString& aCssText)
{
  aCssText.AssignLiteral("@charset \"");
  aCssText.Append(mEncoding);
  aCssText.AppendLiteral("\";");
  return NS_OK;
}

NS_IMETHODIMP
CSSCharsetRuleImpl::SetCssText(const nsAString& aCssText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
CSSCharsetRuleImpl::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  NS_ENSURE_ARG_POINTER(aSheet);

  NS_IF_ADDREF(*aSheet = mSheet);
  return NS_OK;
}

NS_IMETHODIMP
CSSCharsetRuleImpl::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  if (mParentRule) {
    return mParentRule->GetDOMRule(aParentRule);
  }
  *aParentRule = nsnull;
  return NS_OK;
}






class NS_FINAL_CLASS CSSImportRuleImpl : public nsCSSRule,
                                         public nsICSSImportRule,
                                         public nsIDOMCSSImportRule
{
public:
  CSSImportRuleImpl(nsMediaList* aMedia);
  CSSImportRuleImpl(const CSSImportRuleImpl& aCopy);
private:
  ~CSSImportRuleImpl();
public:

  NS_DECL_ISUPPORTS

  DECL_STYLE_RULE_INHERIT

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  virtual PRInt32 GetType() const;
  virtual already_AddRefed<nsICSSRule> Clone() const;

  
  NS_IMETHOD SetURLSpec(const nsString& aURLSpec);
  NS_IMETHOD GetURLSpec(nsString& aURLSpec) const;

  NS_IMETHOD SetMedia(const nsString& aMedia);
  NS_IMETHOD GetMedia(nsString& aMedia) const;

  NS_IMETHOD SetSheet(nsCSSStyleSheet*);
  
  
  NS_DECL_NSIDOMCSSRULE

  
  NS_DECL_NSIDOMCSSIMPORTRULE

protected:
  nsString  mURLSpec;
  nsRefPtr<nsMediaList> mMedia;
  nsRefPtr<nsCSSStyleSheet> mChildSheet;
};

CSSImportRuleImpl::CSSImportRuleImpl(nsMediaList* aMedia)
  : nsCSSRule()
  , mURLSpec()
  , mMedia(aMedia)
{
  
  
  
}

CSSImportRuleImpl::CSSImportRuleImpl(const CSSImportRuleImpl& aCopy)
  : nsCSSRule(aCopy),
    mURLSpec(aCopy.mURLSpec)
{
  nsRefPtr<nsCSSStyleSheet> sheet;
  if (aCopy.mChildSheet) {
    sheet = aCopy.mChildSheet->Clone(nsnull, this, nsnull, nsnull);
  }
  SetSheet(sheet);
  
}

CSSImportRuleImpl::~CSSImportRuleImpl()
{
  if (mChildSheet) {
    mChildSheet->SetOwnerRule(nsnull);
  }
}

NS_IMPL_ADDREF(CSSImportRuleImpl)
NS_IMPL_RELEASE(CSSImportRuleImpl)

DOMCI_DATA(CSSImportRule, CSSImportRuleImpl)


NS_INTERFACE_MAP_BEGIN(CSSImportRuleImpl)
  NS_INTERFACE_MAP_ENTRY(nsICSSImportRule)
  NS_INTERFACE_MAP_ENTRY(nsICSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSImportRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICSSImportRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSImportRule)
NS_INTERFACE_MAP_END

IMPL_STYLE_RULE_INHERIT(CSSImportRuleImpl, nsCSSRule)

#ifdef DEBUG
 void
CSSImportRuleImpl::List(FILE* out, PRInt32 aIndent) const
{
  
  for (PRInt32 indent = aIndent; --indent >= 0; ) fputs("  ", out);

  fputs("@import \"", out);
  fputs(NS_LossyConvertUTF16toASCII(mURLSpec).get(), out);
  fputs("\" ", out);

  nsAutoString mediaText;
  mMedia->GetText(mediaText);
  fputs(NS_LossyConvertUTF16toASCII(mediaText).get(), out);
  fputs("\n", out);
}
#endif

 PRInt32
CSSImportRuleImpl::GetType() const
{
  return nsICSSRule::IMPORT_RULE;
}

 already_AddRefed<nsICSSRule>
CSSImportRuleImpl::Clone() const
{
  nsCOMPtr<nsICSSRule> clone = new CSSImportRuleImpl(*this);
  return clone.forget();
}

NS_IMETHODIMP
CSSImportRuleImpl::SetURLSpec(const nsString& aURLSpec)
{
  mURLSpec = aURLSpec;
  return NS_OK;
}

NS_IMETHODIMP
CSSImportRuleImpl::GetURLSpec(nsString& aURLSpec) const
{
  aURLSpec = mURLSpec;
  return NS_OK;
}

NS_IMETHODIMP
CSSImportRuleImpl::SetMedia(const nsString& aMedia)
{
  if (mMedia) {
    return mMedia->SetText(aMedia);
  } else {
    return NS_OK;
  }
}

NS_IMETHODIMP
CSSImportRuleImpl::GetMedia(nsString& aMedia) const
{
  if (mMedia) {
    return mMedia->GetText(aMedia);
  } else {
    aMedia.Truncate();
    return NS_OK;
  }
}

NS_IMETHODIMP
CSSImportRuleImpl::SetSheet(nsCSSStyleSheet* aSheet)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(aSheet);
  
  
  mChildSheet = aSheet;
  aSheet->SetOwnerRule(this);

  
  nsCOMPtr<nsIDOMMediaList> mediaList;
  rv = mChildSheet->GetMedia(getter_AddRefs(mediaList));
  NS_ENSURE_SUCCESS(rv, rv);
  mMedia = static_cast<nsMediaList*>(mediaList.get());
  
  return NS_OK;
}

nsresult
NS_NewCSSImportRule(nsICSSImportRule** aInstancePtrResult, 
                    const nsString& aURLSpec,
                    nsMediaList* aMedia)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  CSSImportRuleImpl* it = new CSSImportRuleImpl(aMedia);

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  it->SetURLSpec(aURLSpec);
  NS_ADDREF(*aInstancePtrResult = it);
  return NS_OK;
}

NS_IMETHODIMP
CSSImportRuleImpl::GetType(PRUint16* aType)
{
  NS_ENSURE_ARG_POINTER(aType);
  *aType = nsIDOMCSSRule::IMPORT_RULE;
  return NS_OK;
}

NS_IMETHODIMP
CSSImportRuleImpl::GetCssText(nsAString& aCssText)
{
  aCssText.AssignLiteral("@import url(");
  nsStyleUtil::AppendEscapedCSSString(mURLSpec, aCssText);
  aCssText.Append(NS_LITERAL_STRING(")"));
  if (mMedia) {
    nsAutoString mediaText;
    mMedia->GetText(mediaText);
    if (!mediaText.IsEmpty()) {
      aCssText.AppendLiteral(" ");
      aCssText.Append(mediaText);
    }
  }
  aCssText.AppendLiteral(";");
  return NS_OK;
}

NS_IMETHODIMP
CSSImportRuleImpl::SetCssText(const nsAString& aCssText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
CSSImportRuleImpl::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  NS_ENSURE_ARG_POINTER(aSheet);

  NS_IF_ADDREF(*aSheet = mSheet);
  return NS_OK;
}

NS_IMETHODIMP
CSSImportRuleImpl::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  if (mParentRule) {
    return mParentRule->GetDOMRule(aParentRule);
  }
  *aParentRule = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
CSSImportRuleImpl::GetHref(nsAString & aHref)
{
  aHref = mURLSpec;
  return NS_OK;
}

NS_IMETHODIMP
CSSImportRuleImpl::GetMedia(nsIDOMMediaList * *aMedia)
{
  NS_ENSURE_ARG_POINTER(aMedia);

  NS_IF_ADDREF(*aMedia = mMedia);
  return NS_OK;
}

NS_IMETHODIMP
CSSImportRuleImpl::GetStyleSheet(nsIDOMCSSStyleSheet * *aStyleSheet)
{
  NS_ENSURE_ARG_POINTER(aStyleSheet);

  NS_IF_ADDREF(*aStyleSheet = mChildSheet);
  return NS_OK;
}

nsCSSGroupRule::nsCSSGroupRule()
  : nsCSSRule()
  , mRuleCollection(nsnull)
{
}

static PRBool
CloneRuleInto(nsICSSRule* aRule, void* aArray)
{
  nsCOMPtr<nsICSSRule> clone = aRule->Clone();
  static_cast<nsCOMArray<nsICSSRule>*>(aArray)->AppendObject(clone);
  return PR_TRUE;
}

static PRBool
SetParentRuleReference(nsICSSRule* aRule, void* aParentRule)
{
  nsCSSGroupRule* parentRule = static_cast<nsCSSGroupRule*>(aParentRule);
  aRule->SetParentRule(parentRule);
  return PR_TRUE;
}

nsCSSGroupRule::nsCSSGroupRule(const nsCSSGroupRule& aCopy)
  : nsCSSRule(aCopy)
  , mRuleCollection(nsnull) 
{
  const_cast<nsCSSGroupRule&>(aCopy).mRules.EnumerateForwards(CloneRuleInto, &mRules);
  mRules.EnumerateForwards(SetParentRuleReference, this);
}

nsCSSGroupRule::~nsCSSGroupRule()
{
  mRules.EnumerateForwards(SetParentRuleReference, nsnull);
  if (mRuleCollection) {
    mRuleCollection->DropReference();
    NS_RELEASE(mRuleCollection);
  }
}

NS_IMPL_ADDREF(nsCSSGroupRule)
NS_IMPL_RELEASE(nsCSSGroupRule)

IMPL_STYLE_RULE_INHERIT2(nsCSSGroupRule, nsCSSRule)

static PRBool
SetStyleSheetReference(nsICSSRule* aRule, void* aSheet)
{
  nsCSSStyleSheet* sheet = (nsCSSStyleSheet*)aSheet;
  aRule->SetStyleSheet(sheet);
  return PR_TRUE;
}

 void
nsCSSGroupRule::SetStyleSheet(nsCSSStyleSheet* aSheet)
{
  mRules.EnumerateForwards(SetStyleSheetReference, aSheet);
  nsCSSRule::SetStyleSheet(aSheet);
}

#ifdef DEBUG
 void
nsCSSGroupRule::List(FILE* out, PRInt32 aIndent) const
{
  fputs(" {\n", out);

  for (PRInt32 index = 0, count = mRules.Count(); index < count; ++index) {
    mRules.ObjectAt(index)->List(out, aIndent + 1);
  }
  fputs("}\n", out);
}
#endif

NS_IMETHODIMP
nsCSSGroupRule::AppendStyleRule(nsICSSRule* aRule)
{
  mRules.AppendObject(aRule);
  aRule->SetStyleSheet(mSheet);
  aRule->SetParentRule(this);
  if (mSheet) {
    
    
    mSheet->SetModified(PR_TRUE);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsCSSGroupRule::StyleRuleCount(PRInt32& aCount) const
{
  aCount = mRules.Count();
  return NS_OK;
}

NS_IMETHODIMP
nsCSSGroupRule::GetStyleRuleAt(PRInt32 aIndex, nsICSSRule*& aRule) const
{
  if (aIndex < 0 || aIndex >= mRules.Count()) {
    aRule = nsnull;
    return NS_ERROR_ILLEGAL_VALUE;
  }

  NS_ADDREF(aRule = mRules.ObjectAt(aIndex));
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
nsCSSGroupRule::EnumerateRulesForwards(RuleEnumFunc aFunc, void * aData) const
{
  return
    const_cast<nsCSSGroupRule*>(this)->mRules.EnumerateForwards(aFunc, aData);
}







NS_IMETHODIMP
nsCSSGroupRule::DeleteStyleRuleAt(PRUint32 aIndex)
{
  nsICSSRule* rule = mRules.SafeObjectAt(aIndex);
  if (rule) {
    rule->SetStyleSheet(nsnull);
    rule->SetParentRule(nsnull);
  }
  return mRules.RemoveObjectAt(aIndex) ? NS_OK : NS_ERROR_ILLEGAL_VALUE;
}

NS_IMETHODIMP
nsCSSGroupRule::InsertStyleRulesAt(PRUint32 aIndex,
                                   nsCOMArray<nsICSSRule>& aRules)
{
  aRules.EnumerateForwards(SetStyleSheetReference, mSheet);
  aRules.EnumerateForwards(SetParentRuleReference, this);
  if (! mRules.InsertObjectsAt(aRules, aIndex)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsCSSGroupRule::ReplaceStyleRule(nsICSSRule* aOld, nsICSSRule* aNew)
{
  PRInt32 index = mRules.IndexOf(aOld);
  NS_ENSURE_TRUE(index != -1, NS_ERROR_UNEXPECTED);
  mRules.ReplaceObjectAt(aNew, index);
  aNew->SetStyleSheet(mSheet);
  aNew->SetParentRule(this);
  aOld->SetStyleSheet(nsnull);
  aOld->SetParentRule(nsnull);
  return NS_OK;
}

nsresult
nsCSSGroupRule::AppendRulesToCssText(nsAString& aCssText)
{
  aCssText.AppendLiteral(" {\n");

  
  for (PRInt32 index = 0, count = mRules.Count(); index < count; ++index) {
    nsICSSRule* rule = mRules.ObjectAt(index);
    nsCOMPtr<nsIDOMCSSRule> domRule;
    rule->GetDOMRule(getter_AddRefs(domRule));
    if (domRule) {
      nsAutoString cssText;
      domRule->GetCssText(cssText);
      aCssText.Append(NS_LITERAL_STRING("  ") +
                      cssText +
                      NS_LITERAL_STRING("\n"));
    }
  }

  aCssText.AppendLiteral("}");
  
  return NS_OK;
}

nsresult
nsCSSGroupRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  NS_IF_ADDREF(*aSheet = mSheet);
  return NS_OK;
}

nsresult
nsCSSGroupRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  if (mParentRule) {
    return mParentRule->GetDOMRule(aParentRule);
  }
  *aParentRule = nsnull;
  return NS_OK;
}


nsresult
nsCSSGroupRule::GetCssRules(nsIDOMCSSRuleList* *aRuleList)
{
  if (!mRuleCollection) {
    mRuleCollection = new CSSGroupRuleRuleListImpl(this);
    if (!mRuleCollection) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mRuleCollection);
  }

  NS_ADDREF(*aRuleList = mRuleCollection);
  return NS_OK;
}

nsresult
nsCSSGroupRule::InsertRule(const nsAString & aRule, PRUint32 aIndex, PRUint32* _retval)
{
  NS_ENSURE_TRUE(mSheet, NS_ERROR_FAILURE);
  
  if (aIndex > PRUint32(mRules.Count()))
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  NS_ASSERTION(PRUint32(mRules.Count()) <= PR_INT32_MAX,
               "Too many style rules!");

  return mSheet->InsertRuleIntoGroup(aRule, this, aIndex, _retval);
}

nsresult
nsCSSGroupRule::DeleteRule(PRUint32 aIndex)
{
  NS_ENSURE_TRUE(mSheet, NS_ERROR_FAILURE);

  if (aIndex >= PRUint32(mRules.Count()))
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  NS_ASSERTION(PRUint32(mRules.Count()) <= PR_INT32_MAX,
               "Too many style rules!");

  return mSheet->DeleteRuleFromGroup(this, aIndex);
}




nsCSSMediaRule::nsCSSMediaRule()
{
}

nsCSSMediaRule::nsCSSMediaRule(const nsCSSMediaRule& aCopy)
  : nsCSSGroupRule(aCopy)
{
  if (aCopy.mMedia) {
    aCopy.mMedia->Clone(getter_AddRefs(mMedia));
    if (mMedia) {
      
      mMedia->SetStyleSheet(aCopy.mSheet);
    }
  }
}

nsCSSMediaRule::~nsCSSMediaRule()
{
  if (mMedia) {
    mMedia->SetStyleSheet(nsnull);
  }
}

NS_IMPL_ADDREF_INHERITED(nsCSSMediaRule, nsCSSGroupRule)
NS_IMPL_RELEASE_INHERITED(nsCSSMediaRule, nsCSSGroupRule)

DOMCI_DATA(CSSMediaRule, nsCSSMediaRule)


NS_INTERFACE_MAP_BEGIN(nsCSSMediaRule)
  NS_INTERFACE_MAP_ENTRY(nsICSSGroupRule)
  NS_INTERFACE_MAP_ENTRY(nsICSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSMediaRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsCSSGroupRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSMediaRule)
NS_INTERFACE_MAP_END

 void
nsCSSMediaRule::SetStyleSheet(nsCSSStyleSheet* aSheet)
{
  if (mMedia) {
    
    mMedia->SetStyleSheet(nsnull);
    mMedia->SetStyleSheet(aSheet);
  }

  nsCSSGroupRule::SetStyleSheet(aSheet);
}

#ifdef DEBUG
 void
nsCSSMediaRule::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 indent = aIndent; --indent >= 0; ) fputs("  ", out);

  nsAutoString  buffer;

  fputs("@media ", out);

  if (mMedia) {
    nsAutoString mediaText;
    mMedia->GetText(mediaText);
    fputs(NS_LossyConvertUTF16toASCII(mediaText).get(), out);
  }

  nsCSSGroupRule::List(out, aIndent);
}
#endif

 PRInt32
nsCSSMediaRule::GetType() const
{
  return nsICSSRule::MEDIA_RULE;
}

 already_AddRefed<nsICSSRule>
nsCSSMediaRule::Clone() const
{
  nsCOMPtr<nsICSSRule> clone = new nsCSSMediaRule(*this);
  return clone.forget();
}

nsresult
nsCSSMediaRule::SetMedia(nsMediaList* aMedia)
{
  mMedia = aMedia;
  if (aMedia)
    mMedia->SetStyleSheet(mSheet);
  return NS_OK;
}


NS_IMETHODIMP
nsCSSMediaRule::GetType(PRUint16* aType)
{
  *aType = nsIDOMCSSRule::MEDIA_RULE;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSMediaRule::GetCssText(nsAString& aCssText)
{
  aCssText.AssignLiteral("@media ");
  
  if (mMedia) {
    nsAutoString mediaText;
    mMedia->GetText(mediaText);
    aCssText.Append(mediaText);
  }

  return nsCSSGroupRule::AppendRulesToCssText(aCssText);
}

NS_IMETHODIMP
nsCSSMediaRule::SetCssText(const nsAString& aCssText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCSSMediaRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  return nsCSSGroupRule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
nsCSSMediaRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return nsCSSGroupRule::GetParentRule(aParentRule);
}


NS_IMETHODIMP
nsCSSMediaRule::GetMedia(nsIDOMMediaList* *aMedia)
{
  NS_ENSURE_ARG_POINTER(aMedia);
  NS_IF_ADDREF(*aMedia = mMedia);
  return NS_OK;
}

NS_IMETHODIMP
nsCSSMediaRule::GetCssRules(nsIDOMCSSRuleList* *aRuleList)
{
  return nsCSSGroupRule::GetCssRules(aRuleList);
}

NS_IMETHODIMP
nsCSSMediaRule::InsertRule(const nsAString & aRule, PRUint32 aIndex, PRUint32* _retval)
{
  return nsCSSGroupRule::InsertRule(aRule, aIndex, _retval);
}

NS_IMETHODIMP
nsCSSMediaRule::DeleteRule(PRUint32 aIndex)
{
  return nsCSSGroupRule::DeleteRule(aIndex);
}


NS_IMETHODIMP_(PRBool)
nsCSSMediaRule::UseForPresentation(nsPresContext* aPresContext,
                                   nsMediaQueryResultCacheKey& aKey)
{
  if (mMedia) {
    return mMedia->Matches(aPresContext, aKey);
  }
  return PR_TRUE;
}


nsCSSDocumentRule::nsCSSDocumentRule(void)
{
}

nsCSSDocumentRule::nsCSSDocumentRule(const nsCSSDocumentRule& aCopy)
  : nsCSSGroupRule(aCopy)
  , mURLs(new URL(*aCopy.mURLs))
{
}

nsCSSDocumentRule::~nsCSSDocumentRule(void)
{
}

NS_IMPL_ADDREF_INHERITED(nsCSSDocumentRule, nsCSSGroupRule)
NS_IMPL_RELEASE_INHERITED(nsCSSDocumentRule, nsCSSGroupRule)

DOMCI_DATA(CSSMozDocumentRule, nsCSSDocumentRule)


NS_INTERFACE_MAP_BEGIN(nsCSSDocumentRule)
  NS_INTERFACE_MAP_ENTRY(nsICSSGroupRule)
  NS_INTERFACE_MAP_ENTRY(nsICSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSMozDocumentRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsCSSGroupRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSMozDocumentRule)
NS_INTERFACE_MAP_END

#ifdef DEBUG
 void
nsCSSDocumentRule::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 indent = aIndent; --indent >= 0; ) fputs("  ", out);

  nsCAutoString str;
  str.AssignLiteral("@-moz-document ");
  for (URL *url = mURLs; url; url = url->next) {
    switch (url->func) {
      case eURL:
        str.AppendLiteral("url(\"");
        break;
      case eURLPrefix:
        str.AppendLiteral("url-prefix(\"");
        break;
      case eDomain:
        str.AppendLiteral("domain(\"");
        break;
    }
    nsCAutoString escapedURL(url->url);
    escapedURL.ReplaceSubstring("\"", "\\\""); 
    str.Append(escapedURL);
    str.AppendLiteral("\"), ");
  }
  str.Cut(str.Length() - 2, 1); 
  fputs(str.get(), out);

  nsCSSGroupRule::List(out, aIndent);
}
#endif

 PRInt32
nsCSSDocumentRule::GetType() const
{
  return nsICSSRule::DOCUMENT_RULE;
}

 already_AddRefed<nsICSSRule>
nsCSSDocumentRule::Clone() const
{
  nsCOMPtr<nsICSSRule> clone = new nsCSSDocumentRule(*this);
  return clone.forget();
}


NS_IMETHODIMP
nsCSSDocumentRule::GetType(PRUint16* aType)
{
  
  *aType = nsIDOMCSSRule::UNKNOWN_RULE;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSDocumentRule::GetCssText(nsAString& aCssText)
{
  aCssText.AssignLiteral("@-moz-document ");
  for (URL *url = mURLs; url; url = url->next) {
    switch (url->func) {
      case eURL:
        aCssText.AppendLiteral("url(");
        break;
      case eURLPrefix:
        aCssText.AppendLiteral("url-prefix(");
        break;
      case eDomain:
        aCssText.AppendLiteral("domain(");
        break;
    }
    nsStyleUtil::AppendEscapedCSSString(NS_ConvertUTF8toUTF16(url->url),
                                        aCssText);
    aCssText.AppendLiteral("), ");
  }
  aCssText.Cut(aCssText.Length() - 2, 1); 

  return nsCSSGroupRule::AppendRulesToCssText(aCssText);
}

NS_IMETHODIMP
nsCSSDocumentRule::SetCssText(const nsAString& aCssText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCSSDocumentRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  return nsCSSGroupRule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
nsCSSDocumentRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return nsCSSGroupRule::GetParentRule(aParentRule);
}

NS_IMETHODIMP
nsCSSDocumentRule::GetCssRules(nsIDOMCSSRuleList* *aRuleList)
{
  return nsCSSGroupRule::GetCssRules(aRuleList);
}

NS_IMETHODIMP
nsCSSDocumentRule::InsertRule(const nsAString & aRule, PRUint32 aIndex, PRUint32* _retval)
{
  return nsCSSGroupRule::InsertRule(aRule, aIndex, _retval);
}

NS_IMETHODIMP
nsCSSDocumentRule::DeleteRule(PRUint32 aIndex)
{
  return nsCSSGroupRule::DeleteRule(aIndex);
}


NS_IMETHODIMP_(PRBool)
nsCSSDocumentRule::UseForPresentation(nsPresContext* aPresContext,
                                      nsMediaQueryResultCacheKey& aKey)
{
  nsIURI *docURI = aPresContext->Document()->GetDocumentURI();
  nsCAutoString docURISpec;
  if (docURI)
    docURI->GetSpec(docURISpec);

  for (URL *url = mURLs; url; url = url->next) {
    switch (url->func) {
      case eURL: {
        if (docURISpec == url->url)
          return PR_TRUE;
      } break;
      case eURLPrefix: {
        if (StringBeginsWith(docURISpec, url->url))
          return PR_TRUE;
      } break;
      case eDomain: {
        nsCAutoString host;
        if (docURI)
          docURI->GetHost(host);
        PRInt32 lenDiff = host.Length() - url->url.Length();
        if (lenDiff == 0) {
          if (host == url->url)
            return PR_TRUE;
        } else {
          if (StringEndsWith(host, url->url) &&
              host.CharAt(lenDiff - 1) == '.')
            return PR_TRUE;
        }
      } break;
    }
  }

  return PR_FALSE;
}

nsCSSDocumentRule::URL::~URL()
{
  NS_CSS_DELETE_LIST_MEMBER(nsCSSDocumentRule::URL, this, next);
}





namespace mozilla {
namespace css {

class NS_FINAL_CLASS NameSpaceRule : public nsCSSRule,
                                     public nsICSSNameSpaceRule,
                                     public nsIDOMCSSRule
{
public:
  NameSpaceRule();
  NameSpaceRule(const NameSpaceRule& aCopy);
private:
  ~NameSpaceRule();
public:
  NS_DECL_ISUPPORTS

  DECL_STYLE_RULE_INHERIT

  
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  
  virtual PRInt32 GetType() const;
  virtual already_AddRefed<nsICSSRule> Clone() const;

  
  NS_IMETHOD GetPrefix(nsIAtom*& aPrefix) const;
  NS_IMETHOD SetPrefix(nsIAtom* aPrefix);

  NS_IMETHOD GetURLSpec(nsString& aURLSpec) const;
  NS_IMETHOD SetURLSpec(const nsString& aURLSpec);

  
  NS_DECL_NSIDOMCSSRULE
  
protected:
  nsIAtom*  mPrefix;
  nsString  mURLSpec;
};

NameSpaceRule::NameSpaceRule()
  : nsCSSRule(),
    mPrefix(nsnull),
    mURLSpec()
{
}

NameSpaceRule::NameSpaceRule(const NameSpaceRule& aCopy)
  : nsCSSRule(aCopy),
    mPrefix(aCopy.mPrefix),
    mURLSpec(aCopy.mURLSpec)
{
  NS_IF_ADDREF(mPrefix);
}

NameSpaceRule::~NameSpaceRule()
{
  NS_IF_RELEASE(mPrefix);
}

NS_IMPL_ADDREF(NameSpaceRule)
NS_IMPL_RELEASE(NameSpaceRule)


NS_INTERFACE_MAP_BEGIN(NameSpaceRule)
  NS_INTERFACE_MAP_ENTRY(nsICSSNameSpaceRule)
  NS_INTERFACE_MAP_ENTRY(nsICSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICSSNameSpaceRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSNameSpaceRule)
NS_INTERFACE_MAP_END

IMPL_STYLE_RULE_INHERIT(NameSpaceRule, nsCSSRule)

#ifdef DEBUG
 void
NameSpaceRule::List(FILE* out, PRInt32 aIndent) const
{
  for (PRInt32 indent = aIndent; --indent >= 0; ) fputs("  ", out);

  nsAutoString  buffer;

  fputs("@namespace ", out);

  if (mPrefix) {
    mPrefix->ToString(buffer);
    fputs(NS_LossyConvertUTF16toASCII(buffer).get(), out);
    fputs(" ", out);
  }

  fputs("url(", out);
  fputs(NS_LossyConvertUTF16toASCII(mURLSpec).get(), out);
  fputs(")\n", out);
}
#endif

 PRInt32
NameSpaceRule::GetType() const
{
  return nsICSSRule::NAMESPACE_RULE;
}

 already_AddRefed<nsICSSRule>
NameSpaceRule::Clone() const
{
  nsCOMPtr<nsICSSRule> clone = new NameSpaceRule(*this);
  return clone.forget();
}

NS_IMETHODIMP
NameSpaceRule::GetPrefix(nsIAtom*& aPrefix) const
{
  aPrefix = mPrefix;
  NS_IF_ADDREF(aPrefix);
  return NS_OK;
}

NS_IMETHODIMP
NameSpaceRule::SetPrefix(nsIAtom* aPrefix)
{
  NS_IF_RELEASE(mPrefix);
  mPrefix = aPrefix;
  NS_IF_ADDREF(mPrefix);
  return NS_OK;
}

NS_IMETHODIMP
NameSpaceRule::GetURLSpec(nsString& aURLSpec) const
{
  aURLSpec = mURLSpec;
  return NS_OK;
}

NS_IMETHODIMP
NameSpaceRule::SetURLSpec(const nsString& aURLSpec)
{
  mURLSpec = aURLSpec;
  return NS_OK;
}

NS_IMETHODIMP
NameSpaceRule::GetType(PRUint16* aType)
{
  
  *aType = nsIDOMCSSRule::UNKNOWN_RULE;
  return NS_OK;
}

NS_IMETHODIMP
NameSpaceRule::GetCssText(nsAString& aCssText)
{
  aCssText.AssignLiteral("@namespace ");
  if (mPrefix) {
    aCssText.Append(nsDependentAtomString(mPrefix) + NS_LITERAL_STRING(" "));
  }
  aCssText.AppendLiteral("url(");
  nsStyleUtil::AppendEscapedCSSString(mURLSpec, aCssText);
  aCssText.Append(NS_LITERAL_STRING(");"));
  return NS_OK;
}

NS_IMETHODIMP
NameSpaceRule::SetCssText(const nsAString& aCssText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
NameSpaceRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  NS_IF_ADDREF(*aSheet = mSheet);
  return NS_OK;
}

NS_IMETHODIMP
NameSpaceRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  if (mParentRule) {
    return mParentRule->GetDOMRule(aParentRule);
  }
  *aParentRule = nsnull;
  return NS_OK;
}

} 
} 


DOMCI_DATA(CSSNameSpaceRule, css::NameSpaceRule)

nsresult
NS_NewCSSNameSpaceRule(nsICSSNameSpaceRule** aInstancePtrResult,
                       nsIAtom* aPrefix, const nsString& aURLSpec)
{
  if (! aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }

  css::NameSpaceRule* it = new css::NameSpaceRule();

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  it->SetPrefix(aPrefix);
  it->SetURLSpec(aURLSpec);
  NS_ADDREF(*aInstancePtrResult = it);
  return NS_OK;
}










static void
AppendSerializedFontSrc(const nsCSSValue& src, nsAString & aResult NS_OUTPARAM)
{
  NS_PRECONDITION(src.GetUnit() == eCSSUnit_Array,
                  "improper value unit for src:");

  const nsCSSValue::Array& sources = *src.GetArrayValue();
  size_t i = 0;

  while (i < sources.Count()) {
    nsAutoString formats;

    if (sources[i].GetUnit() == eCSSUnit_URL) {
      aResult.AppendLiteral("url(");
      nsDependentString url(sources[i].GetOriginalURLValue());
      nsStyleUtil::AppendEscapedCSSString(url, aResult);
      aResult.AppendLiteral(")");
    } else if (sources[i].GetUnit() == eCSSUnit_Local_Font) {
      aResult.AppendLiteral("local(");
      nsDependentString local(sources[i].GetStringBufferValue());
      nsStyleUtil::AppendEscapedCSSString(local, aResult);
      aResult.AppendLiteral(")");
    } else {
      NS_NOTREACHED("entry in src: descriptor with improper unit");
      i++;
      continue;
    }

    i++;
    formats.Truncate();
    while (i < sources.Count() &&
           sources[i].GetUnit() == eCSSUnit_Font_Format) {
      formats.Append('"');
      formats.Append(sources[i].GetStringBufferValue());
      formats.AppendLiteral("\", ");
      i++;
    }
    if (formats.Length() > 0) {
      formats.Truncate(formats.Length() - 2); 
      aResult.AppendLiteral(" format(");
      aResult.Append(formats);
      aResult.Append(')');
    }
    aResult.AppendLiteral(", ");
  }
  aResult.Truncate(aResult.Length() - 2); 
}


static void
AppendSerializedUnicodePoint(PRUint32 aCode, nsACString &aBuf NS_OUTPARAM)
{
  aBuf.Append(nsPrintfCString("%04X", aCode));
}





static void
AppendSerializedUnicodeRange(nsCSSValue const & aValue,
                             nsAString & aResult NS_OUTPARAM)
{
  NS_PRECONDITION(aValue.GetUnit() == eCSSUnit_Null ||
                  aValue.GetUnit() == eCSSUnit_Array,
                  "improper value unit for unicode-range:");
  aResult.Truncate();
  if (aValue.GetUnit() != eCSSUnit_Array)
    return;

  nsCSSValue::Array const & sources = *aValue.GetArrayValue();
  nsCAutoString buf;

  NS_ABORT_IF_FALSE(sources.Count() % 2 == 0,
                    "odd number of entries in a unicode-range: array");

  for (PRUint32 i = 0; i < sources.Count(); i += 2) {
    PRUint32 min = sources[i].GetIntValue();
    PRUint32 max = sources[i+1].GetIntValue();

    
    buf.AppendLiteral("U+");
    AppendSerializedUnicodePoint(min, buf);

    if (min != max) {
      buf.Append('-');
      AppendSerializedUnicodePoint(max, buf);
    }
    buf.AppendLiteral(", ");
  }
  buf.Truncate(buf.Length() - 2); 
  CopyASCIItoUTF16(buf, aResult);
}



nsCSSValue nsCSSFontFaceStyleDecl::* const
nsCSSFontFaceStyleDecl::Fields[] = {
    &nsCSSFontFaceStyleDecl::mFamily,
    &nsCSSFontFaceStyleDecl::mStyle,
    &nsCSSFontFaceStyleDecl::mWeight,
    &nsCSSFontFaceStyleDecl::mStretch,
    &nsCSSFontFaceStyleDecl::mSrc,
    &nsCSSFontFaceStyleDecl::mUnicodeRange,
    &nsCSSFontFaceStyleDecl::mFontFeatureSettings,
    &nsCSSFontFaceStyleDecl::mFontLanguageOverride
};

DOMCI_DATA(CSSFontFaceStyleDecl, nsCSSFontFaceStyleDecl)


NS_INTERFACE_MAP_BEGIN(nsCSSFontFaceStyleDecl)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSStyleDeclaration)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSFontFaceStyleDecl)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF_USING_AGGREGATOR(nsCSSFontFaceStyleDecl, ContainingRule())
NS_IMPL_RELEASE_USING_AGGREGATOR(nsCSSFontFaceStyleDecl, ContainingRule())


nsresult
nsCSSFontFaceStyleDecl::GetPropertyValue(nsCSSFontDesc aFontDescID,
                                         nsAString & aResult NS_OUTPARAM) const
{
  NS_ENSURE_ARG_RANGE(aFontDescID, eCSSFontDesc_UNKNOWN,
                      eCSSFontDesc_COUNT - 1);

  aResult.Truncate();
  if (aFontDescID == eCSSFontDesc_UNKNOWN)
    return NS_OK;

  const nsCSSValue& val = this->*nsCSSFontFaceStyleDecl::Fields[aFontDescID];

  if (val.GetUnit() == eCSSUnit_Null) {
    
    return NS_OK;
  }

  switch (aFontDescID) {
  case eCSSFontDesc_Family: {
      
      
      
      NS_ASSERTION(val.GetUnit() == eCSSUnit_String, "unexpected unit");
      nsDependentString family(val.GetStringBufferValue());
      nsStyleUtil::AppendEscapedCSSString(family, aResult);
      return NS_OK;
    }

  case eCSSFontDesc_Style:
    val.AppendToString(eCSSProperty_font_style, aResult);
    return NS_OK;

  case eCSSFontDesc_Weight:
    val.AppendToString(eCSSProperty_font_weight, aResult);
    return NS_OK;

  case eCSSFontDesc_Stretch:
    val.AppendToString(eCSSProperty_font_stretch, aResult);
    return NS_OK;

  case eCSSFontDesc_FontFeatureSettings:
    val.AppendToString(eCSSProperty_font_feature_settings, aResult);
    return NS_OK;

  case eCSSFontDesc_FontLanguageOverride:
    val.AppendToString(eCSSProperty_font_language_override, aResult);
    return NS_OK;

  case eCSSFontDesc_Src:
    AppendSerializedFontSrc(val, aResult);
    return NS_OK;

  case eCSSFontDesc_UnicodeRange:
    AppendSerializedUnicodeRange(val, aResult);
    return NS_OK;

  case eCSSFontDesc_UNKNOWN:
  case eCSSFontDesc_COUNT:
    ;
  }
  NS_NOTREACHED("nsCSSFontFaceStyleDecl::GetPropertyValue: "
                "out-of-range value got to the switch");
  return NS_ERROR_INVALID_ARG;
}



NS_IMETHODIMP
nsCSSFontFaceStyleDecl::GetCssText(nsAString & aCssText)
{
  nsAutoString descStr;

  aCssText.Truncate();
  for (nsCSSFontDesc id = nsCSSFontDesc(eCSSFontDesc_UNKNOWN + 1);
       id < eCSSFontDesc_COUNT;
       id = nsCSSFontDesc(id + 1)) {
    if ((this->*nsCSSFontFaceStyleDecl::Fields[id]).GetUnit()
          != eCSSUnit_Null &&
        NS_SUCCEEDED(GetPropertyValue(id, descStr))) {
      NS_ASSERTION(descStr.Length() > 0,
                   "GetCssText: non-null unit, empty property value");
      aCssText.AppendLiteral("  ");
      aCssText.AppendASCII(nsCSSProps::GetStringValue(id).get());
      aCssText.AppendLiteral(": ");
      aCssText.Append(descStr);
      aCssText.AppendLiteral(";\n");
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFontFaceStyleDecl::SetCssText(const nsAString & aCssText)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}


NS_IMETHODIMP
nsCSSFontFaceStyleDecl::GetPropertyValue(const nsAString & propertyName,
                                         nsAString & aResult NS_OUTPARAM)
{
  return GetPropertyValue(nsCSSProps::LookupFontDesc(propertyName), aResult);
}


NS_IMETHODIMP
nsCSSFontFaceStyleDecl::GetPropertyCSSValue(const nsAString & propertyName,
                                            nsIDOMCSSValue **aResult NS_OUTPARAM)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCSSFontFaceStyleDecl::RemoveProperty(const nsAString & propertyName,
                                       nsAString & aResult NS_OUTPARAM)
{
  nsCSSFontDesc descID = nsCSSProps::LookupFontDesc(propertyName);
  NS_ASSERTION(descID >= eCSSFontDesc_UNKNOWN &&
               descID < eCSSFontDesc_COUNT,
               "LookupFontDesc returned value out of range");

  if (descID == eCSSFontDesc_UNKNOWN) {
    aResult.Truncate();
  } else {
    nsresult rv = GetPropertyValue(descID, aResult);
    NS_ENSURE_SUCCESS(rv, rv);
    (this->*nsCSSFontFaceStyleDecl::Fields[descID]).Reset();
  }
  return NS_OK;
}


NS_IMETHODIMP
nsCSSFontFaceStyleDecl::GetPropertyPriority(const nsAString & propertyName,
                                            nsAString & aResult NS_OUTPARAM)
{
  
  aResult.Truncate();
  return NS_OK;
}



NS_IMETHODIMP
nsCSSFontFaceStyleDecl::SetProperty(const nsAString & propertyName,
                                    const nsAString & value,
                                    const nsAString & priority)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}


NS_IMETHODIMP
nsCSSFontFaceStyleDecl::GetLength(PRUint32 *aLength)
{
  PRUint32 len = 0;
  for (nsCSSFontDesc id = nsCSSFontDesc(eCSSFontDesc_UNKNOWN + 1);
       id < eCSSFontDesc_COUNT;
       id = nsCSSFontDesc(id + 1))
    if ((this->*nsCSSFontFaceStyleDecl::Fields[id]).GetUnit() != eCSSUnit_Null)
      len++;

  *aLength = len;
  return NS_OK;
}


NS_IMETHODIMP
nsCSSFontFaceStyleDecl::Item(PRUint32 index, nsAString & aResult NS_OUTPARAM)
 {
  PRInt32 nset = -1;
  for (nsCSSFontDesc id = nsCSSFontDesc(eCSSFontDesc_UNKNOWN + 1);
       id < eCSSFontDesc_COUNT;
       id = nsCSSFontDesc(id + 1)) {
    if ((this->*nsCSSFontFaceStyleDecl::Fields[id]).GetUnit()
        != eCSSUnit_Null) {
      nset++;
      if (nset == PRInt32(index)) {
        aResult.AssignASCII(nsCSSProps::GetStringValue(id).get());
        return NS_OK;
      }
    }
  }
  aResult.Truncate();
  return NS_OK;
}


NS_IMETHODIMP
nsCSSFontFaceStyleDecl::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return ContainingRule()->GetDOMRule(aParentRule);
}






 already_AddRefed<nsICSSRule>
nsCSSFontFaceRule::Clone() const
{
  nsCOMPtr<nsICSSRule> clone = new nsCSSFontFaceRule(*this);
  return clone.forget();
}

NS_IMPL_ADDREF(nsCSSFontFaceRule)
NS_IMPL_RELEASE(nsCSSFontFaceRule)

DOMCI_DATA(CSSFontFaceRule, nsCSSFontFaceRule)


NS_INTERFACE_MAP_BEGIN(nsCSSFontFaceRule)
  NS_INTERFACE_MAP_ENTRY(nsICSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSFontFaceRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICSSRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSFontFaceRule)
NS_INTERFACE_MAP_END

IMPL_STYLE_RULE_INHERIT(nsCSSFontFaceRule, nsCSSRule)

#ifdef DEBUG
void
nsCSSFontFaceRule::List(FILE* out, PRInt32 aIndent) const
{
  nsCString baseInd, descInd;
  for (PRInt32 indent = aIndent; --indent >= 0; ) {
    baseInd.AppendLiteral("  ");
    descInd.AppendLiteral("  ");
  }
  descInd.AppendLiteral("  ");

  nsString descStr;

  fprintf(out, "%s@font-face {\n", baseInd.get());
  for (nsCSSFontDesc id = nsCSSFontDesc(eCSSFontDesc_UNKNOWN + 1);
       id < eCSSFontDesc_COUNT;
       id = nsCSSFontDesc(id + 1))
    if ((mDecl.*nsCSSFontFaceStyleDecl::Fields[id]).GetUnit()
        != eCSSUnit_Null) {
      if (NS_FAILED(mDecl.GetPropertyValue(id, descStr)))
        descStr.AssignLiteral("#<serialization error>");
      else if (descStr.Length() == 0)
        descStr.AssignLiteral("#<serialization missing>");
      fprintf(out, "%s%s: %s\n",
              descInd.get(), nsCSSProps::GetStringValue(id).get(),
              NS_ConvertUTF16toUTF8(descStr).get());
    }
  fprintf(out, "%s}\n", baseInd.get());
}
#endif

 PRInt32
nsCSSFontFaceRule::GetType() const
{
  return nsICSSRule::FONT_FACE_RULE;
}

NS_IMETHODIMP
nsCSSFontFaceRule::GetType(PRUint16* aType)
{
  *aType = nsIDOMCSSRule::FONT_FACE_RULE;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFontFaceRule::GetCssText(nsAString& aCssText)
{
  nsAutoString propText;
  mDecl.GetCssText(propText);

  aCssText.AssignLiteral("@font-face {\n");
  aCssText.Append(propText);
  aCssText.Append('}');
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFontFaceRule::SetCssText(const nsAString& aCssText)
{
  return NS_ERROR_NOT_IMPLEMENTED; 
}

NS_IMETHODIMP
nsCSSFontFaceRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  NS_IF_ADDREF(*aSheet = mSheet);
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFontFaceRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  if (mParentRule) {
    return mParentRule->GetDOMRule(aParentRule);
  }
  *aParentRule = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFontFaceRule::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  NS_IF_ADDREF(*aStyle = &mDecl);
  return NS_OK;
}


void
nsCSSFontFaceRule::SetDesc(nsCSSFontDesc aDescID, nsCSSValue const & aValue)
{
  NS_PRECONDITION(aDescID > eCSSFontDesc_UNKNOWN &&
                  aDescID < eCSSFontDesc_COUNT,
                  "aDescID out of range in nsCSSFontFaceRule::SetDesc");

  mDecl.*nsCSSFontFaceStyleDecl::Fields[aDescID] = aValue;
}

void
nsCSSFontFaceRule::GetDesc(nsCSSFontDesc aDescID, nsCSSValue & aValue)
{
  NS_PRECONDITION(aDescID > eCSSFontDesc_UNKNOWN &&
                  aDescID < eCSSFontDesc_COUNT,
                  "aDescID out of range in nsCSSFontFaceRule::GetDesc");

  aValue = mDecl.*nsCSSFontFaceStyleDecl::Fields[aDescID];
}
