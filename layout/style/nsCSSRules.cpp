






#include "mozilla/Attributes.h"

#include "nsCSSRules.h"
#include "nsCSSValue.h"
#include "mozilla/CSSStyleSheet.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/css/ImportRule.h"
#include "mozilla/css/NameSpaceRule.h"

#include "nsString.h"
#include "nsIAtom.h"

#include "nsCSSProps.h"

#include "nsCOMPtr.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsIMediaList.h"
#include "mozilla/dom/CSSRuleList.h"
#include "nsIDocument.h"
#include "nsPresContext.h"

#include "nsContentUtils.h"
#include "nsError.h"
#include "nsStyleUtil.h"
#include "mozilla/css/Declaration.h"
#include "nsCSSParser.h"
#include "nsDOMClassInfoID.h"
#include "mozilla/dom/CSSStyleDeclarationBinding.h"
#include "StyleRule.h"
#include "nsFont.h"
#include "nsIURI.h"
#include "mozAutoDocUpdate.h"

using namespace mozilla;
using namespace mozilla::dom;

#define IMPL_STYLE_RULE_INHERIT_GET_DOM_RULE_WEAK(class_, super_) \
  /* virtual */ nsIDOMCSSRule* class_::GetDOMRule()               \
  { return this; }                                                \
  /* virtual */ nsIDOMCSSRule* class_::GetExistingDOMRule()       \
  { return this; }
#define IMPL_STYLE_RULE_INHERIT_MAP_RULE_INFO_INTO(class_, super_) \
/* virtual */ void class_::MapRuleInfoInto(nsRuleData* aRuleData) \
  { MOZ_ASSERT(false, "should not be called"); }

#define IMPL_STYLE_RULE_INHERIT(class_, super_) \
IMPL_STYLE_RULE_INHERIT_GET_DOM_RULE_WEAK(class_, super_) \
IMPL_STYLE_RULE_INHERIT_MAP_RULE_INFO_INTO(class_, super_)



namespace mozilla {
namespace css {

CSSStyleSheet*
Rule::GetStyleSheet() const
{
  if (!(mSheet & 0x1)) {
    return reinterpret_cast<CSSStyleSheet*>(mSheet);
  }

  return nullptr;
}

nsHTMLCSSStyleSheet*
Rule::GetHTMLCSSStyleSheet() const
{
  if (mSheet & 0x1) {
    return reinterpret_cast<nsHTMLCSSStyleSheet*>(mSheet & ~uintptr_t(0x1));
  }

  return nullptr;
}

 void
Rule::SetStyleSheet(CSSStyleSheet* aSheet)
{
  
  
  
  mSheet = reinterpret_cast<uintptr_t>(aSheet);
}

void
Rule::SetHTMLCSSStyleSheet(nsHTMLCSSStyleSheet* aSheet)
{
  
  
  
  mSheet = reinterpret_cast<uintptr_t>(aSheet);
  mSheet |= 0x1;
}

nsresult
Rule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  if (mParentRule) {
    NS_IF_ADDREF(*aParentRule = mParentRule->GetDOMRule());
  } else {
    *aParentRule = nullptr;
  }
  return NS_OK;
}

nsresult
Rule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  NS_ENSURE_ARG_POINTER(aSheet);

  NS_IF_ADDREF(*aSheet = GetStyleSheet());
  return NS_OK;
}

css::Rule*
Rule::GetCSSRule()
{
  return this;
}

size_t
Rule::SizeOfCOMArrayElementIncludingThis(css::Rule* aElement,
                                         MallocSizeOf aMallocSizeOf,
                                         void* aData)
{
  return aElement->SizeOfIncludingThis(aMallocSizeOf);
}





class GroupRuleRuleList final : public dom::CSSRuleList
{
public:
  explicit GroupRuleRuleList(GroupRule *aGroupRule);

  virtual CSSStyleSheet* GetParentObject() override;

  virtual nsIDOMCSSRule*
  IndexedGetter(uint32_t aIndex, bool& aFound) override;
  virtual uint32_t
  Length() override;

  void DropReference() { mGroupRule = nullptr; }

private:
  ~GroupRuleRuleList();

private:
  GroupRule* mGroupRule;
};

GroupRuleRuleList::GroupRuleRuleList(GroupRule *aGroupRule)
{
  
  
  mGroupRule = aGroupRule;
}

GroupRuleRuleList::~GroupRuleRuleList()
{
}

CSSStyleSheet*
GroupRuleRuleList::GetParentObject()
{
  if (!mGroupRule) {
    return nullptr;
  }

  return mGroupRule->GetStyleSheet();
}

uint32_t
GroupRuleRuleList::Length()
{
  if (!mGroupRule) {
    return 0;
  }

  return AssertedCast<uint32_t>(mGroupRule->StyleRuleCount());
}

nsIDOMCSSRule*
GroupRuleRuleList::IndexedGetter(uint32_t aIndex, bool& aFound)
{
  aFound = false;

  if (mGroupRule) {
    nsRefPtr<Rule> rule = mGroupRule->GetStyleRuleAt(aIndex);
    if (rule) {
      aFound = true;
      return rule->GetDOMRule();
    }
  }

  return nullptr;
}





CharsetRule::CharsetRule(const nsAString& aEncoding,
                         uint32_t aLineNumber, uint32_t aColumnNumber)
  : Rule(aLineNumber, aColumnNumber),
    mEncoding(aEncoding)
{
}

CharsetRule::CharsetRule(const CharsetRule& aCopy)
  : Rule(aCopy),
    mEncoding(aCopy.mEncoding)
{
}

NS_IMPL_ADDREF(CharsetRule)
NS_IMPL_RELEASE(CharsetRule)


NS_INTERFACE_MAP_BEGIN(CharsetRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSCharsetRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStyleRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSCharsetRule)
NS_INTERFACE_MAP_END

IMPL_STYLE_RULE_INHERIT(CharsetRule, Rule)

#ifdef DEBUG
 void
CharsetRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString str;
  
  for (int32_t indent = aIndent; --indent >= 0; ) {
    str.AppendLiteral("  ");
  }

  str.AppendLiteral("@charset \"");
  AppendUTF16toUTF8(mEncoding, str);
  str.AppendLiteral("\"\n");
  fprintf_stderr(out, "%s", str.get());
}
#endif

 int32_t
CharsetRule::GetType() const
{
  return Rule::CHARSET_RULE;
}

 already_AddRefed<Rule>
CharsetRule::Clone() const
{
  nsRefPtr<Rule> clone = new CharsetRule(*this);
  return clone.forget();
}

NS_IMETHODIMP
CharsetRule::GetEncoding(nsAString& aEncoding)
{
  aEncoding = mEncoding;
  return NS_OK;
}

NS_IMETHODIMP
CharsetRule::SetEncoding(const nsAString& aEncoding)
{
  mEncoding = aEncoding;
  return NS_OK;
}

NS_IMETHODIMP
CharsetRule::GetType(uint16_t* aType)
{
  *aType = nsIDOMCSSRule::CHARSET_RULE;
  return NS_OK;
}

NS_IMETHODIMP
CharsetRule::GetCssText(nsAString& aCssText)
{
  aCssText.AssignLiteral("@charset \"");
  aCssText.Append(mEncoding);
  aCssText.AppendLiteral("\";");
  return NS_OK;
}

NS_IMETHODIMP
CharsetRule::SetCssText(const nsAString& aCssText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
CharsetRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  return Rule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
CharsetRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return Rule::GetParentRule(aParentRule);
}

css::Rule*
CharsetRule::GetCSSRule()
{
  return Rule::GetCSSRule();
}

 size_t
CharsetRule::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this);

  
  
  
}





ImportRule::ImportRule(nsMediaList* aMedia, const nsString& aURLSpec,
                       uint32_t aLineNumber, uint32_t aColumnNumber)
  : Rule(aLineNumber, aColumnNumber)
  , mURLSpec(aURLSpec)
  , mMedia(aMedia)
{
  
  
  
}

ImportRule::ImportRule(const ImportRule& aCopy)
  : Rule(aCopy),
    mURLSpec(aCopy.mURLSpec)
{
  
  
  
  if (aCopy.mChildSheet) {
    nsRefPtr<CSSStyleSheet> sheet =
      aCopy.mChildSheet->Clone(nullptr, this, nullptr, nullptr);
    SetSheet(sheet);
    
  }
}

ImportRule::~ImportRule()
{
  if (mChildSheet) {
    mChildSheet->SetOwnerRule(nullptr);
  }
}

NS_IMPL_CYCLE_COLLECTING_ADDREF(ImportRule)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ImportRule)

NS_IMPL_CYCLE_COLLECTION(ImportRule, mMedia, mChildSheet)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ImportRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSImportRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStyleRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSImportRule)
NS_INTERFACE_MAP_END

IMPL_STYLE_RULE_INHERIT(ImportRule, Rule)

#ifdef DEBUG
 void
ImportRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString str;
  
  for (int32_t indent = aIndent; --indent >= 0; ) {
    str.AppendLiteral("  ");
  }

  str.AppendLiteral("@import \"");
  AppendUTF16toUTF8(mURLSpec, str);
  str.AppendLiteral("\" ");

  nsAutoString mediaText;
  mMedia->GetText(mediaText);
  AppendUTF16toUTF8(mediaText, str);
  str.AppendLiteral("\n");
  fprintf_stderr(out, "%s", str.get());
}
#endif

 int32_t
ImportRule::GetType() const
{
  return Rule::IMPORT_RULE;
}

 already_AddRefed<Rule>
ImportRule::Clone() const
{
  nsRefPtr<Rule> clone = new ImportRule(*this);
  return clone.forget();
}

void
ImportRule::SetSheet(CSSStyleSheet* aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");

  
  mChildSheet = aSheet;
  aSheet->SetOwnerRule(this);

  
  mMedia = mChildSheet->Media();
}

NS_IMETHODIMP
ImportRule::GetType(uint16_t* aType)
{
  NS_ENSURE_ARG_POINTER(aType);
  *aType = nsIDOMCSSRule::IMPORT_RULE;
  return NS_OK;
}

NS_IMETHODIMP
ImportRule::GetCssText(nsAString& aCssText)
{
  aCssText.AssignLiteral("@import url(");
  nsStyleUtil::AppendEscapedCSSString(mURLSpec, aCssText);
  aCssText.Append(')');
  if (mMedia) {
    nsAutoString mediaText;
    mMedia->GetText(mediaText);
    if (!mediaText.IsEmpty()) {
      aCssText.Append(' ');
      aCssText.Append(mediaText);
    }
  }
  aCssText.Append(';');
  return NS_OK;
}

NS_IMETHODIMP
ImportRule::SetCssText(const nsAString& aCssText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
ImportRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  return Rule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
ImportRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return Rule::GetParentRule(aParentRule);
}

css::Rule*
ImportRule::GetCSSRule()
{
  return Rule::GetCSSRule();
}

NS_IMETHODIMP
ImportRule::GetHref(nsAString & aHref)
{
  aHref = mURLSpec;
  return NS_OK;
}

NS_IMETHODIMP
ImportRule::GetMedia(nsIDOMMediaList * *aMedia)
{
  NS_ENSURE_ARG_POINTER(aMedia);

  NS_IF_ADDREF(*aMedia = mMedia);
  return NS_OK;
}

NS_IMETHODIMP
ImportRule::GetStyleSheet(nsIDOMCSSStyleSheet * *aStyleSheet)
{
  NS_ENSURE_ARG_POINTER(aStyleSheet);

  NS_IF_ADDREF(*aStyleSheet = mChildSheet);
  return NS_OK;
}

 size_t
ImportRule::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this);

  
  
  
  
  
  
  
}

GroupRule::GroupRule(uint32_t aLineNumber, uint32_t aColumnNumber)
  : Rule(aLineNumber, aColumnNumber)
{
}

static bool
SetParentRuleReference(Rule* aRule, void* aParentRule)
{
  GroupRule* parentRule = static_cast<GroupRule*>(aParentRule);
  aRule->SetParentRule(parentRule);
  return true;
}

GroupRule::GroupRule(const GroupRule& aCopy)
  : Rule(aCopy)
{
  const_cast<GroupRule&>(aCopy).mRules.EnumerateForwards(GroupRule::CloneRuleInto, &mRules);
  mRules.EnumerateForwards(SetParentRuleReference, this);
}

GroupRule::~GroupRule()
{
  MOZ_ASSERT(!mSheet, "SetStyleSheet should have been called");
  mRules.EnumerateForwards(SetParentRuleReference, nullptr);
  if (mRuleCollection) {
    mRuleCollection->DropReference();
  }
}

NS_IMPL_CYCLE_COLLECTING_ADDREF(GroupRule)
NS_IMPL_CYCLE_COLLECTING_RELEASE(GroupRule)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(GroupRule)
NS_INTERFACE_MAP_END

IMPL_STYLE_RULE_INHERIT_MAP_RULE_INFO_INTO(GroupRule, Rule)

static bool
SetStyleSheetReference(Rule* aRule, void* aSheet)
{
  CSSStyleSheet* sheet = (CSSStyleSheet*)aSheet;
  aRule->SetStyleSheet(sheet);
  return true;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(GroupRule)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(GroupRule)
  tmp->mRules.EnumerateForwards(SetParentRuleReference, nullptr);
  
  
  
  
  
  
  if (tmp->GetStyleSheet()) {
    tmp->mRules.EnumerateForwards(SetStyleSheetReference, nullptr);
  }
  tmp->mRules.Clear();
  if (tmp->mRuleCollection) {
    tmp->mRuleCollection->DropReference();
    tmp->mRuleCollection = nullptr;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(GroupRule)
  const nsCOMArray<Rule>& rules = tmp->mRules;
  for (int32_t i = 0, count = rules.Count(); i < count; ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mRules[i]");
    cb.NoteXPCOMChild(rules[i]->GetExistingDOMRule());
  }
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRuleCollection)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

 void
GroupRule::SetStyleSheet(CSSStyleSheet* aSheet)
{
  
  
  
  
  if (aSheet != GetStyleSheet()) {
    mRules.EnumerateForwards(SetStyleSheetReference, aSheet);
    Rule::SetStyleSheet(aSheet);
  }
}

#ifdef DEBUG
 void
GroupRule::List(FILE* out, int32_t aIndent) const
{
  for (int32_t index = 0, count = mRules.Count(); index < count; ++index) {
    mRules.ObjectAt(index)->List(out, aIndent + 1);
  }
}
#endif

void
GroupRule::AppendStyleRule(Rule* aRule)
{
  mRules.AppendObject(aRule);
  CSSStyleSheet* sheet = GetStyleSheet();
  aRule->SetStyleSheet(sheet);
  aRule->SetParentRule(this);
  if (sheet) {
    sheet->SetModifiedByChildRule();
  }
}

Rule*
GroupRule::GetStyleRuleAt(int32_t aIndex) const
{
  return mRules.SafeObjectAt(aIndex);
}

bool
GroupRule::EnumerateRulesForwards(RuleEnumFunc aFunc, void * aData) const
{
  return
    const_cast<GroupRule*>(this)->mRules.EnumerateForwards(aFunc, aData);
}







nsresult
GroupRule::DeleteStyleRuleAt(uint32_t aIndex)
{
  Rule* rule = mRules.SafeObjectAt(aIndex);
  if (rule) {
    rule->SetStyleSheet(nullptr);
    rule->SetParentRule(nullptr);
  }
  return mRules.RemoveObjectAt(aIndex) ? NS_OK : NS_ERROR_ILLEGAL_VALUE;
}

nsresult
GroupRule::InsertStyleRuleAt(uint32_t aIndex, Rule* aRule)
{
  aRule->SetStyleSheet(GetStyleSheet());
  aRule->SetParentRule(this);
  if (! mRules.InsertObjectAt(aRule, aIndex)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
GroupRule::ReplaceStyleRule(Rule* aOld, Rule* aNew)
{
  int32_t index = mRules.IndexOf(aOld);
  NS_ENSURE_TRUE(index != -1, NS_ERROR_UNEXPECTED);
  mRules.ReplaceObjectAt(aNew, index);
  aNew->SetStyleSheet(GetStyleSheet());
  aNew->SetParentRule(this);
  aOld->SetStyleSheet(nullptr);
  aOld->SetParentRule(nullptr);
  return NS_OK;
}

void
GroupRule::AppendRulesToCssText(nsAString& aCssText)
{
  aCssText.AppendLiteral(" {\n");

  
  for (int32_t index = 0, count = mRules.Count(); index < count; ++index) {
    Rule* rule = mRules.ObjectAt(index);
    nsIDOMCSSRule* domRule = rule->GetDOMRule();
    if (domRule) {
      nsAutoString cssText;
      domRule->GetCssText(cssText);
      aCssText.AppendLiteral("  ");
      aCssText.Append(cssText);
      aCssText.Append('\n');
    }
  }

  aCssText.Append('}');
}


nsresult
GroupRule::GetCssRules(nsIDOMCSSRuleList* *aRuleList)
{
  if (!mRuleCollection) {
    mRuleCollection = new css::GroupRuleRuleList(this);
  }

  NS_ADDREF(*aRuleList = mRuleCollection);
  return NS_OK;
}

nsresult
GroupRule::InsertRule(const nsAString & aRule, uint32_t aIndex, uint32_t* _retval)
{
  CSSStyleSheet* sheet = GetStyleSheet();
  NS_ENSURE_TRUE(sheet, NS_ERROR_FAILURE);
  
  if (aIndex > uint32_t(mRules.Count()))
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  NS_ASSERTION(uint32_t(mRules.Count()) <= INT32_MAX,
               "Too many style rules!");

  return sheet->InsertRuleIntoGroup(aRule, this, aIndex, _retval);
}

nsresult
GroupRule::DeleteRule(uint32_t aIndex)
{
  CSSStyleSheet* sheet = GetStyleSheet();
  NS_ENSURE_TRUE(sheet, NS_ERROR_FAILURE);

  if (aIndex >= uint32_t(mRules.Count()))
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  NS_ASSERTION(uint32_t(mRules.Count()) <= INT32_MAX,
               "Too many style rules!");

  return sheet->DeleteRuleFromGroup(this, aIndex);
}

 size_t
GroupRule::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  return mRules.SizeOfExcludingThis(Rule::SizeOfCOMArrayElementIncludingThis,
                                    aMallocSizeOf);

  
  
  
}





MediaRule::MediaRule(uint32_t aLineNumber, uint32_t aColumnNumber)
  : GroupRule(aLineNumber, aColumnNumber)
{
}

MediaRule::MediaRule(const MediaRule& aCopy)
  : GroupRule(aCopy)
{
  if (aCopy.mMedia) {
    mMedia = aCopy.mMedia->Clone();
    
    mMedia->SetStyleSheet(aCopy.GetStyleSheet());
  }
}

MediaRule::~MediaRule()
{
  if (mMedia) {
    mMedia->SetStyleSheet(nullptr);
  }
}

NS_IMPL_ADDREF_INHERITED(MediaRule, GroupRule)
NS_IMPL_RELEASE_INHERITED(MediaRule, GroupRule)


NS_INTERFACE_MAP_BEGIN(MediaRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSGroupingRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSConditionRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSMediaRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStyleRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSMediaRule)
NS_INTERFACE_MAP_END_INHERITING(GroupRule)

 void
MediaRule::SetStyleSheet(CSSStyleSheet* aSheet)
{
  if (mMedia) {
    
    mMedia->SetStyleSheet(nullptr);
    mMedia->SetStyleSheet(aSheet);
  }

  GroupRule::SetStyleSheet(aSheet);
}

#ifdef DEBUG
 void
MediaRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString indentStr;
  for (int32_t indent = aIndent; --indent >= 0; ) {
    indentStr.AppendLiteral("  ");
  }

  nsAutoCString str(indentStr);
  str.AppendLiteral("@media ");

  if (mMedia) {
    nsAutoString mediaText;
    mMedia->GetText(mediaText);
    AppendUTF16toUTF8(mediaText, str);
  }

  str.AppendLiteral(" {\n");
  fprintf_stderr(out, "%s", str.get());

  GroupRule::List(out, aIndent);

  fprintf_stderr(out, "%s}\n", indentStr.get());
}
#endif

 int32_t
MediaRule::GetType() const
{
  return Rule::MEDIA_RULE;
}

 already_AddRefed<Rule>
MediaRule::Clone() const
{
  nsRefPtr<Rule> clone = new MediaRule(*this);
  return clone.forget();
}

nsresult
MediaRule::SetMedia(nsMediaList* aMedia)
{
  mMedia = aMedia;
  if (aMedia)
    mMedia->SetStyleSheet(GetStyleSheet());
  return NS_OK;
}


NS_IMETHODIMP
MediaRule::GetType(uint16_t* aType)
{
  *aType = nsIDOMCSSRule::MEDIA_RULE;
  return NS_OK;
}

NS_IMETHODIMP
MediaRule::GetCssText(nsAString& aCssText)
{
  aCssText.AssignLiteral("@media ");
  AppendConditionText(aCssText);
  GroupRule::AppendRulesToCssText(aCssText);
  return NS_OK;
}

NS_IMETHODIMP
MediaRule::SetCssText(const nsAString& aCssText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MediaRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  return GroupRule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
MediaRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return GroupRule::GetParentRule(aParentRule);
}

css::Rule*
MediaRule::GetCSSRule()
{
  return Rule::GetCSSRule();
}


NS_IMETHODIMP
MediaRule::GetCssRules(nsIDOMCSSRuleList* *aRuleList)
{
  return GroupRule::GetCssRules(aRuleList);
}

NS_IMETHODIMP
MediaRule::InsertRule(const nsAString & aRule, uint32_t aIndex, uint32_t* _retval)
{
  return GroupRule::InsertRule(aRule, aIndex, _retval);
}

NS_IMETHODIMP
MediaRule::DeleteRule(uint32_t aIndex)
{
  return GroupRule::DeleteRule(aIndex);
}


NS_IMETHODIMP
MediaRule::GetConditionText(nsAString& aConditionText)
{
  aConditionText.Truncate(0);
  AppendConditionText(aConditionText);
  return NS_OK;
}

NS_IMETHODIMP
MediaRule::SetConditionText(const nsAString& aConditionText)
{
  if (!mMedia) {
    nsRefPtr<nsMediaList> media = new nsMediaList();
    media->SetStyleSheet(GetStyleSheet());
    nsresult rv = media->SetMediaText(aConditionText);
    if (NS_SUCCEEDED(rv)) {
      mMedia = media;
    }
    return rv;
  }

  return mMedia->SetMediaText(aConditionText);
}


NS_IMETHODIMP
MediaRule::GetMedia(nsIDOMMediaList* *aMedia)
{
  NS_ENSURE_ARG_POINTER(aMedia);
  NS_IF_ADDREF(*aMedia = mMedia);
  return NS_OK;
}


 bool
MediaRule::UseForPresentation(nsPresContext* aPresContext,
                                   nsMediaQueryResultCacheKey& aKey)
{
  if (mMedia) {
    return mMedia->Matches(aPresContext, &aKey);
  }
  return true;
}

 size_t
MediaRule::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);
  n += GroupRule::SizeOfExcludingThis(aMallocSizeOf);

  
  
  

  return n;
}

void
MediaRule::AppendConditionText(nsAString& aOutput)
{
  if (mMedia) {
    nsAutoString mediaText;
    mMedia->GetText(mediaText);
    aOutput.Append(mediaText);
  }
}

DocumentRule::DocumentRule(uint32_t aLineNumber, uint32_t aColumnNumber)
  : GroupRule(aLineNumber, aColumnNumber)
{
}

DocumentRule::DocumentRule(const DocumentRule& aCopy)
  : GroupRule(aCopy)
  , mURLs(new URL(*aCopy.mURLs))
{
}

DocumentRule::~DocumentRule()
{
}

NS_IMPL_ADDREF_INHERITED(DocumentRule, GroupRule)
NS_IMPL_RELEASE_INHERITED(DocumentRule, GroupRule)


NS_INTERFACE_MAP_BEGIN(DocumentRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSGroupingRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSConditionRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSMozDocumentRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStyleRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSMozDocumentRule)
NS_INTERFACE_MAP_END_INHERITING(GroupRule)

#ifdef DEBUG
 void
DocumentRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString indentStr;
  for (int32_t indent = aIndent; --indent >= 0; ) {
    indentStr.AppendLiteral("  ");
  }

  nsAutoCString str;
  str.AppendLiteral("@-moz-document ");
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
      case eRegExp:
        str.AppendLiteral("regexp(\"");
        break;
    }
    nsAutoCString escapedURL(url->url);
    escapedURL.ReplaceSubstring("\"", "\\\""); 
    str.Append(escapedURL);
    str.AppendLiteral("\"), ");
  }
  str.Cut(str.Length() - 2, 1); 
  fprintf_stderr(out, "%s%s {\n", indentStr.get(), str.get());

  GroupRule::List(out, aIndent);

  fprintf_stderr(out, "%s}\n", indentStr.get());
}
#endif

 int32_t
DocumentRule::GetType() const
{
  return Rule::DOCUMENT_RULE;
}

 already_AddRefed<Rule>
DocumentRule::Clone() const
{
  nsRefPtr<Rule> clone = new DocumentRule(*this);
  return clone.forget();
}


NS_IMETHODIMP
DocumentRule::GetType(uint16_t* aType)
{
  
  *aType = nsIDOMCSSRule::UNKNOWN_RULE;
  return NS_OK;
}

NS_IMETHODIMP
DocumentRule::GetCssText(nsAString& aCssText)
{
  aCssText.AssignLiteral("@-moz-document ");
  AppendConditionText(aCssText);
  GroupRule::AppendRulesToCssText(aCssText);
  return NS_OK;
}

NS_IMETHODIMP
DocumentRule::SetCssText(const nsAString& aCssText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
DocumentRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  return GroupRule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
DocumentRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return GroupRule::GetParentRule(aParentRule);
}

css::Rule*
DocumentRule::GetCSSRule()
{
  return Rule::GetCSSRule();
}


NS_IMETHODIMP
DocumentRule::GetCssRules(nsIDOMCSSRuleList* *aRuleList)
{
  return GroupRule::GetCssRules(aRuleList);
}

NS_IMETHODIMP
DocumentRule::InsertRule(const nsAString & aRule, uint32_t aIndex, uint32_t* _retval)
{
  return GroupRule::InsertRule(aRule, aIndex, _retval);
}

NS_IMETHODIMP
DocumentRule::DeleteRule(uint32_t aIndex)
{
  return GroupRule::DeleteRule(aIndex);
}


NS_IMETHODIMP
DocumentRule::GetConditionText(nsAString& aConditionText)
{
  aConditionText.Truncate(0);
  AppendConditionText(aConditionText);
  return NS_OK;
}

NS_IMETHODIMP
DocumentRule::SetConditionText(const nsAString& aConditionText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


 bool
DocumentRule::UseForPresentation(nsPresContext* aPresContext,
                                 nsMediaQueryResultCacheKey& aKey)
{
  nsIDocument *doc = aPresContext->Document();
  nsIURI *docURI = doc->GetDocumentURI();
  nsAutoCString docURISpec;
  if (docURI)
    docURI->GetSpec(docURISpec);

  for (URL *url = mURLs; url; url = url->next) {
    switch (url->func) {
      case eURL: {
        if (docURISpec == url->url)
          return true;
      } break;
      case eURLPrefix: {
        if (StringBeginsWith(docURISpec, url->url))
          return true;
      } break;
      case eDomain: {
        nsAutoCString host;
        if (docURI)
          docURI->GetHost(host);
        int32_t lenDiff = host.Length() - url->url.Length();
        if (lenDiff == 0) {
          if (host == url->url)
            return true;
        } else {
          if (StringEndsWith(host, url->url) &&
              host.CharAt(lenDiff - 1) == '.')
            return true;
        }
      } break;
      case eRegExp: {
        NS_ConvertUTF8toUTF16 spec(docURISpec);
        NS_ConvertUTF8toUTF16 regex(url->url);
        if (nsContentUtils::IsPatternMatching(spec, regex, doc)) {
          return true;
        }
      } break;
    }
  }

  return false;
}

DocumentRule::URL::~URL()
{
  NS_CSS_DELETE_LIST_MEMBER(DocumentRule::URL, this, next);
}

 size_t
DocumentRule::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);
  n += GroupRule::SizeOfExcludingThis(aMallocSizeOf);

  
  
  

  return n;
}

void
DocumentRule::AppendConditionText(nsAString& aCssText)
{
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
      case eRegExp:
        aCssText.AppendLiteral("regexp(");
        break;
    }
    nsStyleUtil::AppendEscapedCSSString(NS_ConvertUTF8toUTF16(url->url),
                                        aCssText);
    aCssText.AppendLiteral("), ");
  }
  aCssText.Truncate(aCssText.Length() - 2); 
}





NameSpaceRule::NameSpaceRule(nsIAtom* aPrefix, const nsString& aURLSpec,
                             uint32_t aLineNumber, uint32_t aColumnNumber)
  : Rule(aLineNumber, aColumnNumber),
    mPrefix(aPrefix),
    mURLSpec(aURLSpec)
{
}

NameSpaceRule::NameSpaceRule(const NameSpaceRule& aCopy)
  : Rule(aCopy),
    mPrefix(aCopy.mPrefix),
    mURLSpec(aCopy.mURLSpec)
{
}

NameSpaceRule::~NameSpaceRule()
{
}

NS_IMPL_ADDREF(NameSpaceRule)
NS_IMPL_RELEASE(NameSpaceRule)


NS_INTERFACE_MAP_BEGIN(NameSpaceRule)
  if (aIID.Equals(NS_GET_IID(css::NameSpaceRule))) {
    *aInstancePtr = this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStyleRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSNameSpaceRule)
NS_INTERFACE_MAP_END

IMPL_STYLE_RULE_INHERIT(NameSpaceRule, Rule)

#ifdef DEBUG
 void
NameSpaceRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString str;
  for (int32_t indent = aIndent; --indent >= 0; ) {
    str.AppendLiteral("  ");
  }

  nsAutoString  buffer;

  str.AppendLiteral("@namespace ");

  if (mPrefix) {
    mPrefix->ToString(buffer);
    AppendUTF16toUTF8(buffer, str);
    str.Append(' ');
  }

  str.AppendLiteral("url(\"");
  AppendUTF16toUTF8(mURLSpec, str);
  str.AppendLiteral("\")\n");
  fprintf_stderr(out, "%s", str.get());
}
#endif

 int32_t
NameSpaceRule::GetType() const
{
  return Rule::NAMESPACE_RULE;
}

 already_AddRefed<Rule>
NameSpaceRule::Clone() const
{
  nsRefPtr<Rule> clone = new NameSpaceRule(*this);
  return clone.forget();
}

NS_IMETHODIMP
NameSpaceRule::GetType(uint16_t* aType)
{
  *aType = nsIDOMCSSRule::NAMESPACE_RULE;
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
  aCssText.AppendLiteral(");");
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
  return Rule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
NameSpaceRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return Rule::GetParentRule(aParentRule);
}

css::Rule*
NameSpaceRule::GetCSSRule()
{
  return Rule::GetCSSRule();
}

 size_t
NameSpaceRule::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this);

  
  
  
  
}


} 
} 






nsCSSValue CSSFontFaceDescriptors::* const
CSSFontFaceDescriptors::Fields[] = {
#define CSS_FONT_DESC(name_, method_) &CSSFontFaceDescriptors::m##method_,
#include "nsCSSFontDescList.h"
#undef CSS_FONT_DESC
};

const nsCSSValue&
CSSFontFaceDescriptors::Get(nsCSSFontDesc aFontDescID) const
{
  MOZ_ASSERT(aFontDescID > eCSSFontDesc_UNKNOWN &&
             aFontDescID < eCSSFontDesc_COUNT);
  return this->*CSSFontFaceDescriptors::Fields[aFontDescID];
}

nsCSSValue&
CSSFontFaceDescriptors::Get(nsCSSFontDesc aFontDescID)
{
  MOZ_ASSERT(aFontDescID > eCSSFontDesc_UNKNOWN &&
             aFontDescID < eCSSFontDesc_COUNT);
  return this->*CSSFontFaceDescriptors::Fields[aFontDescID];
}


NS_INTERFACE_MAP_BEGIN(nsCSSFontFaceStyleDecl)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSStyleDeclaration)
  NS_INTERFACE_MAP_ENTRY(nsICSSDeclaration)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  
  
  if (aIID.Equals(NS_GET_IID(nsCycleCollectionISupports)) ||
      aIID.Equals(NS_GET_IID(nsXPCOMCycleCollectionParticipant))) {
    return ContainingRule()->QueryInterface(aIID, aInstancePtr);
  }
  else
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF_USING_AGGREGATOR(nsCSSFontFaceStyleDecl, ContainingRule())
NS_IMPL_RELEASE_USING_AGGREGATOR(nsCSSFontFaceStyleDecl, ContainingRule())


nsresult
nsCSSFontFaceStyleDecl::GetPropertyValue(nsCSSFontDesc aFontDescID,
                                         nsAString & aResult) const
{
  NS_ENSURE_ARG_RANGE(aFontDescID, eCSSFontDesc_UNKNOWN,
                      eCSSFontDesc_COUNT - 1);

  aResult.Truncate();
  if (aFontDescID == eCSSFontDesc_UNKNOWN)
    return NS_OK;

  const nsCSSValue& val = mDescriptors.Get(aFontDescID);

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
    val.AppendToString(eCSSProperty_font_style, aResult,
                        nsCSSValue::eNormalized);
    return NS_OK;

  case eCSSFontDesc_Weight:
    val.AppendToString(eCSSProperty_font_weight, aResult,
                       nsCSSValue::eNormalized);
    return NS_OK;

  case eCSSFontDesc_Stretch:
    val.AppendToString(eCSSProperty_font_stretch, aResult,
                       nsCSSValue::eNormalized);
    return NS_OK;

  case eCSSFontDesc_FontFeatureSettings:
    nsStyleUtil::AppendFontFeatureSettings(val, aResult);
    return NS_OK;

  case eCSSFontDesc_FontLanguageOverride:
    val.AppendToString(eCSSProperty_font_language_override, aResult,
                       nsCSSValue::eNormalized);
    return NS_OK;

  case eCSSFontDesc_Src:
    nsStyleUtil::AppendSerializedFontSrc(val, aResult);
    return NS_OK;

  case eCSSFontDesc_UnicodeRange:
    nsStyleUtil::AppendUnicodeRange(val, aResult);
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
    if (mDescriptors.Get(id).GetUnit() != eCSSUnit_Null &&
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
                                         nsAString & aResult)
{
  return GetPropertyValue(nsCSSProps::LookupFontDesc(propertyName), aResult);
}

NS_IMETHODIMP
nsCSSFontFaceStyleDecl::GetAuthoredPropertyValue(const nsAString& propertyName,
                                                 nsAString& aResult)
{
  
  
  return GetPropertyValue(nsCSSProps::LookupFontDesc(propertyName), aResult);
}


already_AddRefed<dom::CSSValue>
nsCSSFontFaceStyleDecl::GetPropertyCSSValue(const nsAString & propertyName,
                                            ErrorResult& aRv)
{
  
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return nullptr;
}


NS_IMETHODIMP
nsCSSFontFaceStyleDecl::RemoveProperty(const nsAString & propertyName,
                                       nsAString & aResult)
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
    mDescriptors.Get(descID).Reset();
  }
  return NS_OK;
}


NS_IMETHODIMP
nsCSSFontFaceStyleDecl::GetPropertyPriority(const nsAString & propertyName,
                                            nsAString & aResult)
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
nsCSSFontFaceStyleDecl::GetLength(uint32_t *aLength)
{
  uint32_t len = 0;
  for (nsCSSFontDesc id = nsCSSFontDesc(eCSSFontDesc_UNKNOWN + 1);
       id < eCSSFontDesc_COUNT;
       id = nsCSSFontDesc(id + 1))
    if (mDescriptors.Get(id).GetUnit() != eCSSUnit_Null)
      len++;

  *aLength = len;
  return NS_OK;
}


NS_IMETHODIMP
nsCSSFontFaceStyleDecl::Item(uint32_t aIndex, nsAString& aReturn)
{
  bool found;
  IndexedGetter(aIndex, found, aReturn);
  if (!found) {
    aReturn.Truncate();
  }
  return NS_OK;
}

void
nsCSSFontFaceStyleDecl::IndexedGetter(uint32_t index, bool& aFound, nsAString & aResult)
{
  int32_t nset = -1;
  for (nsCSSFontDesc id = nsCSSFontDesc(eCSSFontDesc_UNKNOWN + 1);
       id < eCSSFontDesc_COUNT;
       id = nsCSSFontDesc(id + 1)) {
    if (mDescriptors.Get(id).GetUnit() != eCSSUnit_Null) {
      nset++;
      if (nset == int32_t(index)) {
        aFound = true;
        aResult.AssignASCII(nsCSSProps::GetStringValue(id).get());
        return;
      }
    }
  }
  aFound = false;
}


NS_IMETHODIMP
nsCSSFontFaceStyleDecl::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  NS_IF_ADDREF(*aParentRule = ContainingRule()->GetDOMRule());
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFontFaceStyleDecl::GetPropertyValue(const nsCSSProperty aPropID,
                                         nsAString& aValue)
{
  return
    GetPropertyValue(NS_ConvertUTF8toUTF16(nsCSSProps::GetStringValue(aPropID)),
                     aValue);
}

NS_IMETHODIMP
nsCSSFontFaceStyleDecl::SetPropertyValue(const nsCSSProperty aPropID,
                                         const nsAString& aValue)
{
  return SetProperty(NS_ConvertUTF8toUTF16(nsCSSProps::GetStringValue(aPropID)),
                     aValue, EmptyString());
}

nsINode*
nsCSSFontFaceStyleDecl::GetParentObject()
{
  return ContainingRule()->GetDocument();
}

JSObject*
nsCSSFontFaceStyleDecl::WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto)
{
  return mozilla::dom::CSSStyleDeclarationBinding::Wrap(cx, this, aGivenProto);
}





 already_AddRefed<css::Rule>
nsCSSFontFaceRule::Clone() const
{
  nsRefPtr<css::Rule> clone = new nsCSSFontFaceRule(*this);
  return clone.forget();
}

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsCSSFontFaceRule)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsCSSFontFaceRule)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsCSSFontFaceRule)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsCSSFontFaceRule)
  
  
  
  tmp->mDecl.TraceWrapper(aCallbacks, aClosure);
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsCSSFontFaceRule)
  
  
  
  tmp->mDecl.ReleaseWrapper(static_cast<nsISupports*>(p));
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsCSSFontFaceRule)
  
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsCSSFontFaceRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSFontFaceRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStyleRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSFontFaceRule)
NS_INTERFACE_MAP_END

IMPL_STYLE_RULE_INHERIT(nsCSSFontFaceRule, Rule)

#ifdef DEBUG
void
nsCSSFontFaceRule::List(FILE* out, int32_t aIndent) const
{
  nsCString baseInd, descInd;
  for (int32_t indent = aIndent; --indent >= 0; ) {
    baseInd.AppendLiteral("  ");
    descInd.AppendLiteral("  ");
  }
  descInd.AppendLiteral("  ");

  nsString descStr;

  fprintf_stderr(out, "%s@font-face {\n", baseInd.get());
  for (nsCSSFontDesc id = nsCSSFontDesc(eCSSFontDesc_UNKNOWN + 1);
       id < eCSSFontDesc_COUNT;
       id = nsCSSFontDesc(id + 1))
    if (mDecl.mDescriptors.Get(id).GetUnit() != eCSSUnit_Null) {
      if (NS_FAILED(mDecl.GetPropertyValue(id, descStr)))
        descStr.AssignLiteral("#<serialization error>");
      else if (descStr.Length() == 0)
        descStr.AssignLiteral("#<serialization missing>");
      fprintf_stderr(out, "%s%s: %s\n",
                     descInd.get(), nsCSSProps::GetStringValue(id).get(),
                     NS_ConvertUTF16toUTF8(descStr).get());
    }
  fprintf_stderr(out, "%s}\n", baseInd.get());
}
#endif

 int32_t
nsCSSFontFaceRule::GetType() const
{
  return Rule::FONT_FACE_RULE;
}

NS_IMETHODIMP
nsCSSFontFaceRule::GetType(uint16_t* aType)
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
  return Rule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
nsCSSFontFaceRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return Rule::GetParentRule(aParentRule);
}

css::Rule*
nsCSSFontFaceRule::GetCSSRule()
{
  return Rule::GetCSSRule();
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

  

  mDecl.mDescriptors.Get(aDescID) = aValue;
}

void
nsCSSFontFaceRule::GetDesc(nsCSSFontDesc aDescID, nsCSSValue & aValue)
{
  NS_PRECONDITION(aDescID > eCSSFontDesc_UNKNOWN &&
                  aDescID < eCSSFontDesc_COUNT,
                  "aDescID out of range in nsCSSFontFaceRule::GetDesc");

  aValue = mDecl.mDescriptors.Get(aDescID);
}

 size_t
nsCSSFontFaceRule::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this);

  
  
  
}






 already_AddRefed<css::Rule>
nsCSSFontFeatureValuesRule::Clone() const
{
  nsRefPtr<css::Rule> clone = new nsCSSFontFeatureValuesRule(*this);
  return clone.forget();
}

NS_IMPL_ADDREF(nsCSSFontFeatureValuesRule)
NS_IMPL_RELEASE(nsCSSFontFeatureValuesRule)


NS_INTERFACE_MAP_BEGIN(nsCSSFontFeatureValuesRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSFontFeatureValuesRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStyleRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSFontFeatureValuesRule)
NS_INTERFACE_MAP_END

IMPL_STYLE_RULE_INHERIT(nsCSSFontFeatureValuesRule, Rule)

static void
FeatureValuesToString(
  const nsTArray<gfxFontFeatureValueSet::FeatureValues>& aFeatureValues,
  nsAString& aOutStr)
{
  uint32_t i, n;

  
  n = aFeatureValues.Length();
  for (i = 0; i < n; i++) {
    const gfxFontFeatureValueSet::FeatureValues& fv = aFeatureValues[i];

    
    aOutStr.AppendLiteral("  @");
    nsAutoString functAlt;
    nsStyleUtil::GetFunctionalAlternatesName(fv.alternate, functAlt);
    aOutStr.Append(functAlt);
    aOutStr.AppendLiteral(" {");

    
    uint32_t j, numValues = fv.valuelist.Length();
    for (j = 0; j < numValues; j++) {
      aOutStr.Append(' ');
      const gfxFontFeatureValueSet::ValueList& vlist = fv.valuelist[j];
      nsStyleUtil::AppendEscapedCSSIdent(vlist.name, aOutStr);
      aOutStr.Append(':');

      uint32_t k, numSelectors = vlist.featureSelectors.Length();
      for (k = 0; k < numSelectors; k++) {
        aOutStr.Append(' ');
        aOutStr.AppendInt(vlist.featureSelectors[k]);
      }

      aOutStr.Append(';');
    }
    aOutStr.AppendLiteral(" }\n");
  }
}

static void
FontFeatureValuesRuleToString(
  const mozilla::FontFamilyList& aFamilyList,
  const nsTArray<gfxFontFeatureValueSet::FeatureValues>& aFeatureValues,
  nsAString& aOutStr)
{
  aOutStr.AssignLiteral("@font-feature-values ");
  nsAutoString familyListStr, valueTextStr;
  nsStyleUtil::AppendEscapedCSSFontFamilyList(aFamilyList, familyListStr);
  aOutStr.Append(familyListStr);
  aOutStr.AppendLiteral(" {\n");
  FeatureValuesToString(aFeatureValues, valueTextStr);
  aOutStr.Append(valueTextStr);
  aOutStr.Append('}');
}

#ifdef DEBUG
void
nsCSSFontFeatureValuesRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoString text;
  FontFeatureValuesRuleToString(mFamilyList, mFeatureValues, text);
  NS_ConvertUTF16toUTF8 utf8(text);

  
  char* indent = new char[(aIndent + 1) * 2];
  int32_t i;
  for (i = 1; i < (aIndent + 1) * 2 - 1; i++) {
    indent[i] = 0x20;
  }
  indent[0] = 0xa;
  indent[aIndent * 2 + 1] = 0;
  utf8.ReplaceSubstring("\n", indent);
  delete [] indent;

  nsAutoCString indentStr;
  for (i = aIndent; --i >= 0; ) {
    indentStr.AppendLiteral("  ");
  }
  fprintf_stderr(out, "%s%s\n", indentStr.get(), utf8.get());
}
#endif

 int32_t
nsCSSFontFeatureValuesRule::GetType() const
{
  return Rule::FONT_FEATURE_VALUES_RULE;
}

NS_IMETHODIMP
nsCSSFontFeatureValuesRule::GetType(uint16_t* aType)
{
  *aType = nsIDOMCSSRule::FONT_FEATURE_VALUES_RULE;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFontFeatureValuesRule::GetCssText(nsAString& aCssText)
{
  FontFeatureValuesRuleToString(mFamilyList, mFeatureValues, aCssText);
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFontFeatureValuesRule::SetCssText(const nsAString& aCssText)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCSSFontFeatureValuesRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  return Rule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
nsCSSFontFeatureValuesRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return Rule::GetParentRule(aParentRule);
}

css::Rule*
nsCSSFontFeatureValuesRule::GetCSSRule()
{
  return Rule::GetCSSRule();
}

NS_IMETHODIMP
nsCSSFontFeatureValuesRule::GetFontFamily(nsAString& aFamilyListStr)
{
  nsStyleUtil::AppendEscapedCSSFontFamilyList(mFamilyList, aFamilyListStr);
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFontFeatureValuesRule::SetFontFamily(const nsAString& aFontFamily)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCSSFontFeatureValuesRule::GetValueText(nsAString& aValueText)
{
  FeatureValuesToString(mFeatureValues, aValueText);
  return NS_OK;
}

NS_IMETHODIMP
nsCSSFontFeatureValuesRule::SetValueText(const nsAString& aValueText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

struct MakeFamilyArray {
  explicit MakeFamilyArray(nsTArray<nsString>& aFamilyArray)
    : familyArray(aFamilyArray), hasGeneric(false)
  {}

  static bool
  AddFamily(const nsString& aFamily, bool aGeneric, void* aData)
  {
    MakeFamilyArray *familyArr = reinterpret_cast<MakeFamilyArray*> (aData);
    if (!aGeneric && !aFamily.IsEmpty()) {
      familyArr->familyArray.AppendElement(aFamily);
    }
    if (aGeneric) {
      familyArr->hasGeneric = true;
    }
    return true;
  }

  nsTArray<nsString>& familyArray;
  bool hasGeneric;
};

void
nsCSSFontFeatureValuesRule::SetFamilyList(
  const mozilla::FontFamilyList& aFamilyList)
{
  mFamilyList = aFamilyList;
}

void
nsCSSFontFeatureValuesRule::AddValueList(int32_t aVariantAlternate,
                     nsTArray<gfxFontFeatureValueSet::ValueList>& aValueList)
{
  uint32_t i, len = mFeatureValues.Length();
  bool foundAlternate = false;

  
  for (i = 0; i < len; i++) {
    gfxFontFeatureValueSet::FeatureValues& f = mFeatureValues.ElementAt(i);

    if (f.alternate == uint32_t(aVariantAlternate)) {
      f.valuelist.AppendElements(aValueList);
      foundAlternate = true;
      break;
    }
  }

  
  if (!foundAlternate) {
    gfxFontFeatureValueSet::FeatureValues &f = *mFeatureValues.AppendElement();
    f.alternate = aVariantAlternate;
    f.valuelist.AppendElements(aValueList);
  }
}

size_t
nsCSSFontFeatureValuesRule::SizeOfIncludingThis(
  MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this);
}





nsCSSKeyframeStyleDeclaration::nsCSSKeyframeStyleDeclaration(nsCSSKeyframeRule *aRule)
  : mRule(aRule)
{
}

nsCSSKeyframeStyleDeclaration::~nsCSSKeyframeStyleDeclaration()
{
  NS_ASSERTION(!mRule, "DropReference not called.");
}

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsCSSKeyframeStyleDeclaration)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsCSSKeyframeStyleDeclaration)

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(nsCSSKeyframeStyleDeclaration)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsCSSKeyframeStyleDeclaration)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
NS_INTERFACE_MAP_END_INHERITING(nsDOMCSSDeclaration)

css::Declaration*
nsCSSKeyframeStyleDeclaration::GetCSSDeclaration(Operation aOperation)
{
  if (mRule) {
    return mRule->Declaration();
  } else {
    return nullptr;
  }
}

void
nsCSSKeyframeStyleDeclaration::GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv)
{
  GetCSSParsingEnvironmentForRule(mRule, aCSSParseEnv);
}

NS_IMETHODIMP
nsCSSKeyframeStyleDeclaration::GetParentRule(nsIDOMCSSRule **aParent)
{
  NS_ENSURE_ARG_POINTER(aParent);

  NS_IF_ADDREF(*aParent = mRule);
  return NS_OK;
}

nsresult
nsCSSKeyframeStyleDeclaration::SetCSSDeclaration(css::Declaration* aDecl)
{
  MOZ_ASSERT(aDecl, "must be non-null");
  mRule->ChangeDeclaration(aDecl);
  return NS_OK;
}

nsIDocument*
nsCSSKeyframeStyleDeclaration::DocToUpdate()
{
  return nullptr;
}

nsINode*
nsCSSKeyframeStyleDeclaration::GetParentObject()
{
  return mRule ? mRule->GetDocument() : nullptr;
}





nsCSSKeyframeRule::nsCSSKeyframeRule(const nsCSSKeyframeRule& aCopy)
  
  : Rule(aCopy)
  , mKeys(aCopy.mKeys)
  , mDeclaration(new css::Declaration(*aCopy.mDeclaration))
{
}

nsCSSKeyframeRule::~nsCSSKeyframeRule()
{
  if (mDOMDeclaration) {
    mDOMDeclaration->DropReference();
  }
}

 already_AddRefed<css::Rule>
nsCSSKeyframeRule::Clone() const
{
  nsRefPtr<css::Rule> clone = new nsCSSKeyframeRule(*this);
  return clone.forget();
}

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsCSSKeyframeRule)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsCSSKeyframeRule)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsCSSKeyframeRule)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsCSSKeyframeRule)
  if (tmp->mDOMDeclaration) {
    tmp->mDOMDeclaration->DropReference();
    tmp->mDOMDeclaration = nullptr;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsCSSKeyframeRule)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDOMDeclaration)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsCSSKeyframeRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozCSSKeyframeRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStyleRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozCSSKeyframeRule)
NS_INTERFACE_MAP_END

IMPL_STYLE_RULE_INHERIT_GET_DOM_RULE_WEAK(nsCSSKeyframeRule, Rule)

 void
nsCSSKeyframeRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  
  
  

  
  NS_ASSERTION(!mDeclaration->HasImportantData(),
               "Keyframe rules has !important data");

  mDeclaration->MapNormalRuleInfoInto(aRuleData);
}

#ifdef DEBUG
void
nsCSSKeyframeRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString str;
  for (int32_t index = aIndent; --index >= 0; ) {
    str.AppendLiteral("  ");
  }

  nsAutoString tmp;
  DoGetKeyText(tmp);
  AppendUTF16toUTF8(tmp, str);
  str.AppendLiteral(" { ");
  mDeclaration->ToString(tmp);
  AppendUTF16toUTF8(tmp, str);
  str.AppendLiteral("}\n");
  fprintf_stderr(out, "%s", str.get());
}
#endif

 int32_t
nsCSSKeyframeRule::GetType() const
{
  return Rule::KEYFRAME_RULE;
}

NS_IMETHODIMP
nsCSSKeyframeRule::GetType(uint16_t* aType)
{
  *aType = nsIDOMCSSRule::KEYFRAME_RULE;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSKeyframeRule::GetCssText(nsAString& aCssText)
{
  DoGetKeyText(aCssText);
  aCssText.AppendLiteral(" { ");
  nsAutoString tmp;
  mDeclaration->ToString(tmp);
  aCssText.Append(tmp);
  aCssText.AppendLiteral(" }");
  return NS_OK;
}

NS_IMETHODIMP
nsCSSKeyframeRule::SetCssText(const nsAString& aCssText)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCSSKeyframeRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  return Rule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
nsCSSKeyframeRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return Rule::GetParentRule(aParentRule);
}

css::Rule*
nsCSSKeyframeRule::GetCSSRule()
{
  return Rule::GetCSSRule();
}

NS_IMETHODIMP
nsCSSKeyframeRule::GetKeyText(nsAString& aKeyText)
{
  DoGetKeyText(aKeyText);
  return NS_OK;
}

void
nsCSSKeyframeRule::DoGetKeyText(nsAString& aKeyText) const
{
  aKeyText.Truncate();
  uint32_t i = 0, i_end = mKeys.Length();
  MOZ_ASSERT(i_end != 0, "must have some keys");
  for (;;) {
    aKeyText.AppendFloat(mKeys[i] * 100.0f);
    aKeyText.Append(char16_t('%'));
    if (++i == i_end) {
      break;
    }
    aKeyText.AppendLiteral(", ");
  }
}

NS_IMETHODIMP
nsCSSKeyframeRule::SetKeyText(const nsAString& aKeyText)
{
  nsCSSParser parser;

  InfallibleTArray<float> newSelectors;
  
  if (!parser.ParseKeyframeSelectorString(aKeyText, nullptr, 0, newSelectors)) {
    
    return NS_OK;
  }

  nsIDocument* doc = GetDocument();
  MOZ_AUTO_DOC_UPDATE(doc, UPDATE_STYLE, true);

  newSelectors.SwapElements(mKeys);

  CSSStyleSheet* sheet = GetStyleSheet();
  if (sheet) {
    sheet->SetModifiedByChildRule();

    if (doc) {
      doc->StyleRuleChanged(sheet, this, this);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCSSKeyframeRule::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  if (!mDOMDeclaration) {
    mDOMDeclaration = new nsCSSKeyframeStyleDeclaration(this);
  }
  NS_ADDREF(*aStyle = mDOMDeclaration);
  return NS_OK;
}

void
nsCSSKeyframeRule::ChangeDeclaration(css::Declaration* aDeclaration)
{
  
  
  
  nsIDocument* doc = GetDocument();
  MOZ_AUTO_DOC_UPDATE(doc, UPDATE_STYLE, true);

  
  
  if (aDeclaration != mDeclaration) {
    mDeclaration = aDeclaration;
  }

  CSSStyleSheet* sheet = GetStyleSheet();
  if (sheet) {
    sheet->SetModifiedByChildRule();

    if (doc) {
      doc->StyleRuleChanged(sheet, this, this);
    }
  }
}

 size_t
nsCSSKeyframeRule::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this);

  
  
  
  
  
}






nsCSSKeyframesRule::nsCSSKeyframesRule(const nsCSSKeyframesRule& aCopy)
  
  
  
  : GroupRule(aCopy),
    mName(aCopy.mName)
{
}

nsCSSKeyframesRule::~nsCSSKeyframesRule()
{
}

 already_AddRefed<css::Rule>
nsCSSKeyframesRule::Clone() const
{
  nsRefPtr<css::Rule> clone = new nsCSSKeyframesRule(*this);
  return clone.forget();
}

NS_IMPL_ADDREF_INHERITED(nsCSSKeyframesRule, css::GroupRule)
NS_IMPL_RELEASE_INHERITED(nsCSSKeyframesRule, css::GroupRule)


NS_INTERFACE_MAP_BEGIN(nsCSSKeyframesRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozCSSKeyframesRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStyleRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozCSSKeyframesRule)
NS_INTERFACE_MAP_END_INHERITING(GroupRule)

#ifdef DEBUG
void
nsCSSKeyframesRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString indentStr;
  for (int32_t indent = aIndent; --indent >= 0; ) {
    indentStr.AppendLiteral("  ");
  }

  fprintf_stderr(out, "%s@keyframes %s {\n",
                 indentStr.get(), NS_ConvertUTF16toUTF8(mName).get());

  GroupRule::List(out, aIndent);

  fprintf_stderr(out, "%s}\n", indentStr.get());
}
#endif

 int32_t
nsCSSKeyframesRule::GetType() const
{
  return Rule::KEYFRAMES_RULE;
}

NS_IMETHODIMP
nsCSSKeyframesRule::GetType(uint16_t* aType)
{
  *aType = nsIDOMCSSRule::KEYFRAMES_RULE;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSKeyframesRule::GetCssText(nsAString& aCssText)
{
  aCssText.AssignLiteral("@keyframes ");
  aCssText.Append(mName);
  aCssText.AppendLiteral(" {\n");
  nsAutoString tmp;
  for (uint32_t i = 0, i_end = mRules.Count(); i != i_end; ++i) {
    static_cast<nsCSSKeyframeRule*>(mRules[i])->GetCssText(tmp);
    aCssText.Append(tmp);
    aCssText.Append('\n');
  }
  aCssText.Append('}');
  return NS_OK;
}

NS_IMETHODIMP
nsCSSKeyframesRule::SetCssText(const nsAString& aCssText)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCSSKeyframesRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  return GroupRule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
nsCSSKeyframesRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return GroupRule::GetParentRule(aParentRule);
}

css::Rule*
nsCSSKeyframesRule::GetCSSRule()
{
  return GroupRule::GetCSSRule();
}

NS_IMETHODIMP
nsCSSKeyframesRule::GetName(nsAString& aName)
{
  aName = mName;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSKeyframesRule::SetName(const nsAString& aName)
{
  if (mName == aName) {
    return NS_OK;
  }

  nsIDocument* doc = GetDocument();
  MOZ_AUTO_DOC_UPDATE(doc, UPDATE_STYLE, true);

  mName = aName;

  CSSStyleSheet* sheet = GetStyleSheet();
  if (sheet) {
    sheet->SetModifiedByChildRule();

    if (doc) {
      doc->StyleRuleChanged(sheet, this, this);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCSSKeyframesRule::GetCssRules(nsIDOMCSSRuleList* *aRuleList)
{
  return GroupRule::GetCssRules(aRuleList);
}

NS_IMETHODIMP
nsCSSKeyframesRule::AppendRule(const nsAString& aRule)
{
  
  
  
  nsCSSParser parser;

  
  nsRefPtr<nsCSSKeyframeRule> rule =
    parser.ParseKeyframeRule(aRule, nullptr, 0);
  if (rule) {
    nsIDocument* doc = GetDocument();
    MOZ_AUTO_DOC_UPDATE(doc, UPDATE_STYLE, true);

    AppendStyleRule(rule);

    CSSStyleSheet* sheet = GetStyleSheet();
    if (sheet) {
      sheet->SetModifiedByChildRule();

      if (doc) {
        doc->StyleRuleChanged(sheet, this, this);
      }
    }
  }

  return NS_OK;
}

static const uint32_t RULE_NOT_FOUND = uint32_t(-1);

uint32_t
nsCSSKeyframesRule::FindRuleIndexForKey(const nsAString& aKey)
{
  nsCSSParser parser;

  InfallibleTArray<float> keys;
  
  if (parser.ParseKeyframeSelectorString(aKey, nullptr, 0, keys)) {
    
    
    
    
    
    for (uint32_t i = mRules.Count(); i-- != 0; ) {
      if (static_cast<nsCSSKeyframeRule*>(mRules[i])->GetKeys() == keys) {
        return i;
      }
    }
  }

  return RULE_NOT_FOUND;
}

NS_IMETHODIMP
nsCSSKeyframesRule::DeleteRule(const nsAString& aKey)
{
  uint32_t index = FindRuleIndexForKey(aKey);
  if (index != RULE_NOT_FOUND) {
    nsIDocument* doc = GetDocument();
    MOZ_AUTO_DOC_UPDATE(doc, UPDATE_STYLE, true);

    mRules.RemoveObjectAt(index);

    CSSStyleSheet* sheet = GetStyleSheet();
    if (sheet) {
      sheet->SetModifiedByChildRule();

      if (doc) {
        doc->StyleRuleChanged(sheet, this, this);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsCSSKeyframesRule::FindRule(const nsAString& aKey,
                             nsIDOMMozCSSKeyframeRule** aResult)
{
  uint32_t index = FindRuleIndexForKey(aKey);
  if (index == RULE_NOT_FOUND) {
    *aResult = nullptr;
  } else {
    NS_ADDREF(*aResult = static_cast<nsCSSKeyframeRule*>(mRules[index]));
  }
  return NS_OK;
}


 bool
nsCSSKeyframesRule::UseForPresentation(nsPresContext* aPresContext,
                                       nsMediaQueryResultCacheKey& aKey)
{
  MOZ_ASSERT(false, "should not be called");
  return false;
}

 size_t
nsCSSKeyframesRule::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);
  n += GroupRule::SizeOfExcludingThis(aMallocSizeOf);

  
  
  

  return n;
}





nsCSSPageStyleDeclaration::nsCSSPageStyleDeclaration(nsCSSPageRule* aRule)
  : mRule(aRule)
{
}

nsCSSPageStyleDeclaration::~nsCSSPageStyleDeclaration()
{
  NS_ASSERTION(!mRule, "DropReference not called.");
}

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsCSSPageStyleDeclaration)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsCSSPageStyleDeclaration)

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(nsCSSPageStyleDeclaration)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsCSSPageStyleDeclaration)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
NS_INTERFACE_MAP_END_INHERITING(nsDOMCSSDeclaration)

css::Declaration*
nsCSSPageStyleDeclaration::GetCSSDeclaration(Operation aOperation)
{
  if (mRule) {
    return mRule->Declaration();
  } else {
    return nullptr;
  }
}

void
nsCSSPageStyleDeclaration::GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv)
{
  GetCSSParsingEnvironmentForRule(mRule, aCSSParseEnv);
}

NS_IMETHODIMP
nsCSSPageStyleDeclaration::GetParentRule(nsIDOMCSSRule** aParent)
{
  NS_ENSURE_ARG_POINTER(aParent);

  NS_IF_ADDREF(*aParent = mRule);
  return NS_OK;
}

nsresult
nsCSSPageStyleDeclaration::SetCSSDeclaration(css::Declaration* aDecl)
{
  MOZ_ASSERT(aDecl, "must be non-null");
  mRule->ChangeDeclaration(aDecl);
  return NS_OK;
}

nsIDocument*
nsCSSPageStyleDeclaration::DocToUpdate()
{
  return nullptr;
}

nsINode*
nsCSSPageStyleDeclaration::GetParentObject()
{
  return mRule ? mRule->GetDocument() : nullptr;
}





nsCSSPageRule::nsCSSPageRule(const nsCSSPageRule& aCopy)
  
  : Rule(aCopy)
  , mDeclaration(new css::Declaration(*aCopy.mDeclaration))
{
}

nsCSSPageRule::~nsCSSPageRule()
{
  if (mDOMDeclaration) {
    mDOMDeclaration->DropReference();
  }
}

 already_AddRefed<css::Rule>
nsCSSPageRule::Clone() const
{
  nsRefPtr<css::Rule> clone = new nsCSSPageRule(*this);
  return clone.forget();
}

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsCSSPageRule)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsCSSPageRule)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsCSSPageRule)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsCSSPageRule)
  if (tmp->mDOMDeclaration) {
    tmp->mDOMDeclaration->DropReference();
    tmp->mDOMDeclaration = nullptr;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsCSSPageRule)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDOMDeclaration)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsCSSPageRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSPageRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStyleRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSPageRule)
NS_INTERFACE_MAP_END

IMPL_STYLE_RULE_INHERIT_GET_DOM_RULE_WEAK(nsCSSPageRule, Rule)

#ifdef DEBUG
void
nsCSSPageRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString str;
  for (int32_t indent = aIndent; --indent >= 0; ) {
    str.AppendLiteral("  ");
  }

  str.AppendLiteral("@page { ");
  nsAutoString tmp;
  mDeclaration->ToString(tmp);
  AppendUTF16toUTF8(tmp, str);
  str.AppendLiteral("}\n");
  fprintf_stderr(out, "%s", str.get());
}
#endif

 int32_t
nsCSSPageRule::GetType() const
{
  return Rule::PAGE_RULE;
}

NS_IMETHODIMP
nsCSSPageRule::GetType(uint16_t* aType)
{
  *aType = nsIDOMCSSRule::PAGE_RULE;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSPageRule::GetCssText(nsAString& aCssText)
{
  aCssText.AppendLiteral("@page { ");
  nsAutoString tmp;
  mDeclaration->ToString(tmp);
  aCssText.Append(tmp);
  aCssText.AppendLiteral(" }");
  return NS_OK;
}

NS_IMETHODIMP
nsCSSPageRule::SetCssText(const nsAString& aCssText)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCSSPageRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  return Rule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
nsCSSPageRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return Rule::GetParentRule(aParentRule);
}

css::Rule*
nsCSSPageRule::GetCSSRule()
{
  return Rule::GetCSSRule();
}

css::ImportantRule*
nsCSSPageRule::GetImportantRule()
{
  if (!mDeclaration->HasImportantData()) {
    return nullptr;
  }
  if (!mImportantRule) {
    mImportantRule = new css::ImportantRule(mDeclaration);
  }
  return mImportantRule;
}

 void
nsCSSPageRule::MapRuleInfoInto(nsRuleData* aRuleData)
{
  mDeclaration->MapNormalRuleInfoInto(aRuleData);
}

NS_IMETHODIMP
nsCSSPageRule::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  if (!mDOMDeclaration) {
    mDOMDeclaration = new nsCSSPageStyleDeclaration(this);
  }
  NS_ADDREF(*aStyle = mDOMDeclaration);
  return NS_OK;
}

void
nsCSSPageRule::ChangeDeclaration(css::Declaration* aDeclaration)
{
  mImportantRule = nullptr;
  
  
  if (aDeclaration != mDeclaration) {
    mDeclaration = aDeclaration;
  }

  CSSStyleSheet* sheet = GetStyleSheet();
  if (sheet) {
    sheet->SetModifiedByChildRule();
  }
}

 size_t
nsCSSPageRule::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this);
}

namespace mozilla {

CSSSupportsRule::CSSSupportsRule(bool aConditionMet,
                                 const nsString& aCondition,
                                 uint32_t aLineNumber, uint32_t aColumnNumber)
  : css::GroupRule(aLineNumber, aColumnNumber)
  , mUseGroup(aConditionMet)
  , mCondition(aCondition)
{
}

CSSSupportsRule::~CSSSupportsRule()
{
}

CSSSupportsRule::CSSSupportsRule(const CSSSupportsRule& aCopy)
  : css::GroupRule(aCopy),
    mUseGroup(aCopy.mUseGroup),
    mCondition(aCopy.mCondition)
{
}

#ifdef DEBUG
 void
CSSSupportsRule::List(FILE* out, int32_t aIndent) const
{
  nsAutoCString indentStr;
  for (int32_t indent = aIndent; --indent >= 0; ) {
    indentStr.AppendLiteral("  ");
  }

  fprintf_stderr(out, "%s@supports %s {\n",
                 indentStr.get(), NS_ConvertUTF16toUTF8(mCondition).get());

  css::GroupRule::List(out, aIndent);

  fprintf_stderr(out, "%s}\n", indentStr.get());
}
#endif

 int32_t
CSSSupportsRule::GetType() const
{
  return Rule::SUPPORTS_RULE;
}

 already_AddRefed<mozilla::css::Rule>
CSSSupportsRule::Clone() const
{
  nsRefPtr<css::Rule> clone = new CSSSupportsRule(*this);
  return clone.forget();
}

 bool
CSSSupportsRule::UseForPresentation(nsPresContext* aPresContext,
                                   nsMediaQueryResultCacheKey& aKey)
{
  return mUseGroup;
}

NS_IMPL_ADDREF_INHERITED(CSSSupportsRule, css::GroupRule)
NS_IMPL_RELEASE_INHERITED(CSSSupportsRule, css::GroupRule)


NS_INTERFACE_MAP_BEGIN(CSSSupportsRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSGroupingRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSConditionRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSSupportsRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStyleRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSSupportsRule)
NS_INTERFACE_MAP_END_INHERITING(GroupRule)


NS_IMETHODIMP
CSSSupportsRule::GetType(uint16_t* aType)
{
  *aType = nsIDOMCSSRule::SUPPORTS_RULE;
  return NS_OK;
}

NS_IMETHODIMP
CSSSupportsRule::GetCssText(nsAString& aCssText)
{
  aCssText.AssignLiteral("@supports ");
  aCssText.Append(mCondition);
  css::GroupRule::AppendRulesToCssText(aCssText);
  return NS_OK;
}

NS_IMETHODIMP
CSSSupportsRule::SetCssText(const nsAString& aCssText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
CSSSupportsRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  return css::GroupRule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
CSSSupportsRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return css::GroupRule::GetParentRule(aParentRule);
}

css::Rule*
CSSSupportsRule::GetCSSRule()
{
  return css::GroupRule::GetCSSRule();
}


NS_IMETHODIMP
CSSSupportsRule::GetCssRules(nsIDOMCSSRuleList* *aRuleList)
{
  return css::GroupRule::GetCssRules(aRuleList);
}

NS_IMETHODIMP
CSSSupportsRule::InsertRule(const nsAString & aRule, uint32_t aIndex, uint32_t* _retval)
{
  return css::GroupRule::InsertRule(aRule, aIndex, _retval);
}

NS_IMETHODIMP
CSSSupportsRule::DeleteRule(uint32_t aIndex)
{
  return css::GroupRule::DeleteRule(aIndex);
}


NS_IMETHODIMP
CSSSupportsRule::GetConditionText(nsAString& aConditionText)
{
  aConditionText.Assign(mCondition);
  return NS_OK;
}

NS_IMETHODIMP
CSSSupportsRule::SetConditionText(const nsAString& aConditionText)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

 size_t
CSSSupportsRule::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);
  n += css::GroupRule::SizeOfExcludingThis(aMallocSizeOf);
  n += mCondition.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
  return n;
}

} 





nsCSSCounterStyleRule::nsCSSCounterStyleRule(const nsCSSCounterStyleRule& aCopy)
  : Rule(aCopy)
  , mName(aCopy.mName)
  , mGeneration(aCopy.mGeneration)
{
  for (size_t i = 0; i < ArrayLength(mValues); ++i) {
    mValues[i] = aCopy.mValues[i];
  }
}

nsCSSCounterStyleRule::~nsCSSCounterStyleRule()
{
}

 already_AddRefed<css::Rule>
nsCSSCounterStyleRule::Clone() const
{
  nsRefPtr<css::Rule> clone = new nsCSSCounterStyleRule(*this);
  return clone.forget();
}

nsCSSCounterStyleRule::Getter const
nsCSSCounterStyleRule::kGetters[] = {
#define CSS_COUNTER_DESC(name_, method_) &nsCSSCounterStyleRule::Get##method_,
#include "nsCSSCounterDescList.h"
#undef CSS_COUNTER_DESC
};

NS_IMPL_ADDREF(nsCSSCounterStyleRule)
NS_IMPL_RELEASE(nsCSSCounterStyleRule)


NS_INTERFACE_MAP_BEGIN(nsCSSCounterStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIStyleRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRule)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSCounterStyleRule)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStyleRule)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSCounterStyleRule)
NS_INTERFACE_MAP_END

IMPL_STYLE_RULE_INHERIT(nsCSSCounterStyleRule, css::Rule)

#ifdef DEBUG
void
nsCSSCounterStyleRule::List(FILE* out, int32_t aIndent) const
{
  nsCString baseInd, descInd;
  for (int32_t indent = aIndent; --indent >= 0; ) {
    baseInd.AppendLiteral("  ");
  }
  descInd = baseInd;
  descInd.AppendLiteral("  ");

  fprintf_stderr(out, "%s@counter-style %s (rev.%u) {\n",
                 baseInd.get(), NS_ConvertUTF16toUTF8(mName).get(),
                 mGeneration);
  
  fprintf_stderr(out, "%s}\n", baseInd.get());
}
#endif

 int32_t
nsCSSCounterStyleRule::GetType() const
{
  return Rule::COUNTER_STYLE_RULE;
}


NS_IMETHODIMP
nsCSSCounterStyleRule::GetType(uint16_t* aType)
{
  *aType = nsIDOMCSSRule::COUNTER_STYLE_RULE;
  return NS_OK;
}

NS_IMETHODIMP
nsCSSCounterStyleRule::GetCssText(nsAString& aCssText)
{
  aCssText.AssignLiteral(MOZ_UTF16("@counter-style "));
  nsStyleUtil::AppendEscapedCSSIdent(mName, aCssText);
  aCssText.AppendLiteral(MOZ_UTF16(" {\n"));
  for (nsCSSCounterDesc id = nsCSSCounterDesc(0);
       id < eCSSCounterDesc_COUNT;
       id = nsCSSCounterDesc(id + 1)) {
    if (mValues[id].GetUnit() != eCSSUnit_Null) {
      nsAutoString tmp;
      (this->*kGetters[id])(tmp);
      aCssText.AppendLiteral(MOZ_UTF16("  "));
      AppendASCIItoUTF16(nsCSSProps::GetStringValue(id), aCssText);
      aCssText.AppendLiteral(MOZ_UTF16(": "));
      aCssText.Append(tmp);
      aCssText.AppendLiteral(MOZ_UTF16(";\n"));
    }
  }
  aCssText.AppendLiteral(MOZ_UTF16("}"));
  return NS_OK;
}

NS_IMETHODIMP
nsCSSCounterStyleRule::SetCssText(const nsAString& aCssText)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCSSCounterStyleRule::GetParentStyleSheet(nsIDOMCSSStyleSheet** aSheet)
{
  return Rule::GetParentStyleSheet(aSheet);
}

NS_IMETHODIMP
nsCSSCounterStyleRule::GetParentRule(nsIDOMCSSRule** aParentRule)
{
  return Rule::GetParentRule(aParentRule);
}

css::Rule*
nsCSSCounterStyleRule::GetCSSRule()
{
  return Rule::GetCSSRule();
}


NS_IMETHODIMP
nsCSSCounterStyleRule::GetName(nsAString& aName)
{
  aName.Truncate();
  nsStyleUtil::AppendEscapedCSSIdent(mName, aName);
  return NS_OK;
}

NS_IMETHODIMP
nsCSSCounterStyleRule::SetName(const nsAString& aName)
{
  nsCSSParser parser;
  nsAutoString name;
  if (parser.ParseCounterStyleName(aName, nullptr, name)) {
    nsIDocument* doc = GetDocument();
    MOZ_AUTO_DOC_UPDATE(doc, UPDATE_STYLE, true);

    mName = name;

    CSSStyleSheet* sheet = GetStyleSheet();
    if (sheet) {
      sheet->SetModifiedByChildRule();
      if (doc) {
        doc->StyleRuleChanged(sheet, this, this);
      }
    }
  }
  return NS_OK;
}

int32_t
nsCSSCounterStyleRule::GetSystem() const
{
  const nsCSSValue& system = GetDesc(eCSSCounterDesc_System);
  switch (system.GetUnit()) {
    case eCSSUnit_Enumerated:
      return system.GetIntValue();
    case eCSSUnit_Pair:
      return system.GetPairValue().mXValue.GetIntValue();
    default:
      return NS_STYLE_COUNTER_SYSTEM_SYMBOLIC;
  }
}

const nsCSSValue&
nsCSSCounterStyleRule::GetSystemArgument() const
{
  const nsCSSValue& system = GetDesc(eCSSCounterDesc_System);
  MOZ_ASSERT(system.GetUnit() == eCSSUnit_Pair,
             "Invalid system value");
  return system.GetPairValue().mYValue;
}

void
nsCSSCounterStyleRule::SetDesc(nsCSSCounterDesc aDescID, const nsCSSValue& aValue)
{
  MOZ_ASSERT(aDescID >= 0 && aDescID < eCSSCounterDesc_COUNT,
             "descriptor ID out of range");

  nsIDocument* doc = GetDocument();
  MOZ_AUTO_DOC_UPDATE(doc, UPDATE_STYLE, true);

  mValues[aDescID] = aValue;
  mGeneration++;

  CSSStyleSheet* sheet = GetStyleSheet();
  if (sheet) {
    sheet->SetModifiedByChildRule();
    if (doc) {
      doc->StyleRuleChanged(sheet, this, this);
    }
  }
}

NS_IMETHODIMP
nsCSSCounterStyleRule::GetSystem(nsAString& aSystem)
{
  const nsCSSValue& value = GetDesc(eCSSCounterDesc_System);
  if (value.GetUnit() == eCSSUnit_Null) {
    aSystem.Truncate();
    return NS_OK;
  }

  aSystem = NS_ConvertASCIItoUTF16(nsCSSProps::ValueToKeyword(
          GetSystem(), nsCSSProps::kCounterSystemKTable));
  if (value.GetUnit() == eCSSUnit_Pair) {
    aSystem.Append(' ');
    GetSystemArgument().AppendToString(
        eCSSProperty_UNKNOWN, aSystem, nsCSSValue::eNormalized);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsCSSCounterStyleRule::GetSymbols(nsAString& aSymbols)
{
  const nsCSSValue& value = GetDesc(eCSSCounterDesc_Symbols);

  aSymbols.Truncate();
  if (value.GetUnit() == eCSSUnit_List) {
    for (const nsCSSValueList* item = value.GetListValue();
         item; item = item->mNext) {
      item->mValue.AppendToString(eCSSProperty_UNKNOWN,
                                  aSymbols,
                                  nsCSSValue::eNormalized);
      if (item->mNext) {
        aSymbols.Append(' ');
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsCSSCounterStyleRule::GetAdditiveSymbols(nsAString& aSymbols)
{
  const nsCSSValue& value = GetDesc(eCSSCounterDesc_AdditiveSymbols);

  aSymbols.Truncate();
  if (value.GetUnit() == eCSSUnit_PairList) {
    for (const nsCSSValuePairList* item = value.GetPairListValue();
         item; item = item->mNext) {
      item->mXValue.AppendToString(eCSSProperty_UNKNOWN,
                                   aSymbols, nsCSSValue::eNormalized);
      aSymbols.Append(' ');
      item->mYValue.AppendToString(eCSSProperty_UNKNOWN,
                                   aSymbols, nsCSSValue::eNormalized);
      if (item->mNext) {
        aSymbols.AppendLiteral(", ");
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsCSSCounterStyleRule::GetRange(nsAString& aRange)
{
  const nsCSSValue& value = GetDesc(eCSSCounterDesc_Range);

  switch (value.GetUnit()) {
    case eCSSUnit_Auto:
      aRange.AssignLiteral(MOZ_UTF16("auto"));
      break;

    case eCSSUnit_PairList:
      aRange.Truncate();
      for (const nsCSSValuePairList* item = value.GetPairListValue();
          item; item = item->mNext) {
        const nsCSSValue& lower = item->mXValue;
        const nsCSSValue& upper = item->mYValue;
        if (lower.GetUnit() == eCSSUnit_Enumerated) {
          NS_ASSERTION(lower.GetIntValue() ==
                       NS_STYLE_COUNTER_RANGE_INFINITE,
                       "Unrecognized keyword");
          aRange.AppendLiteral("infinite");
        } else {
          aRange.AppendInt(lower.GetIntValue());
        }
        aRange.Append(' ');
        if (upper.GetUnit() == eCSSUnit_Enumerated) {
          NS_ASSERTION(upper.GetIntValue() ==
                       NS_STYLE_COUNTER_RANGE_INFINITE,
                       "Unrecognized keyword");
          aRange.AppendLiteral("infinite");
        } else {
          aRange.AppendInt(upper.GetIntValue());
        }
        if (item->mNext) {
          aRange.AppendLiteral(", ");
        }
      }
      break;

    default:
      aRange.Truncate();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsCSSCounterStyleRule::GetSpeakAs(nsAString& aSpeakAs)
{
  const nsCSSValue& value = GetDesc(eCSSCounterDesc_SpeakAs);

  switch (value.GetUnit()) {
    case eCSSUnit_Enumerated:
      switch (value.GetIntValue()) {
        case NS_STYLE_COUNTER_SPEAKAS_BULLETS:
          aSpeakAs.AssignLiteral(MOZ_UTF16("bullets"));
          break;
        case NS_STYLE_COUNTER_SPEAKAS_NUMBERS:
          aSpeakAs.AssignLiteral(MOZ_UTF16("numbers"));
          break;
        case NS_STYLE_COUNTER_SPEAKAS_WORDS:
          aSpeakAs.AssignLiteral(MOZ_UTF16("words"));
          break;
        case NS_STYLE_COUNTER_SPEAKAS_SPELL_OUT:
          aSpeakAs.AssignLiteral(MOZ_UTF16("spell-out"));
          break;
        default:
          NS_NOTREACHED("Unknown speech synthesis");
      }
      break;

    case eCSSUnit_Auto:
    case eCSSUnit_Ident:
      aSpeakAs.Truncate();
      value.AppendToString(eCSSProperty_UNKNOWN,
                           aSpeakAs, nsCSSValue::eNormalized);
      break;

    default:
      aSpeakAs.Truncate();
  }
  return NS_OK;
}

nsresult
nsCSSCounterStyleRule::GetDescriptor(nsCSSCounterDesc aDescID,
                                     nsAString& aValue)
{
  NS_ASSERTION(aDescID == eCSSCounterDesc_Negative ||
               aDescID == eCSSCounterDesc_Prefix ||
               aDescID == eCSSCounterDesc_Suffix ||
               aDescID == eCSSCounterDesc_Pad ||
               aDescID == eCSSCounterDesc_Fallback,
               "Unexpected descriptor");
  const nsCSSValue& value = GetDesc(aDescID);
  aValue.Truncate();
  if (value.GetUnit() != eCSSUnit_Null) {
    value.AppendToString(
        eCSSProperty_UNKNOWN, aValue, nsCSSValue::eNormalized);
  }
  return NS_OK;
}

#define CSS_COUNTER_DESC_GETTER(name_)                    \
NS_IMETHODIMP                                             \
nsCSSCounterStyleRule::Get##name_(nsAString& a##name_)    \
{                                                         \
  return GetDescriptor(eCSSCounterDesc_##name_, a##name_);\
}
CSS_COUNTER_DESC_GETTER(Negative)
CSS_COUNTER_DESC_GETTER(Prefix)
CSS_COUNTER_DESC_GETTER(Suffix)
CSS_COUNTER_DESC_GETTER(Pad)
CSS_COUNTER_DESC_GETTER(Fallback)
#undef CSS_COUNTER_DESC_GETTER

 bool
nsCSSCounterStyleRule::CheckDescValue(int32_t aSystem,
                                      nsCSSCounterDesc aDescID,
                                      const nsCSSValue& aValue)
{
  switch (aDescID) {
    case eCSSCounterDesc_System:
      if (aValue.GetUnit() != eCSSUnit_Pair) {
        return aValue.GetIntValue() == aSystem;
      } else {
        return aValue.GetPairValue().mXValue.GetIntValue() == aSystem;
      }

    case eCSSCounterDesc_Symbols:
      switch (aSystem) {
        case NS_STYLE_COUNTER_SYSTEM_NUMERIC:
        case NS_STYLE_COUNTER_SYSTEM_ALPHABETIC:
          
          return aValue.GetListValue()->mNext;
        case NS_STYLE_COUNTER_SYSTEM_EXTENDS:
          
          return false;
        default:
          return true;
      }

    case eCSSCounterDesc_AdditiveSymbols:
      switch (aSystem) {
        case NS_STYLE_COUNTER_SYSTEM_EXTENDS:
          return false;
        default:
          return true;
      }

    default:
      return true;
  }
}

nsresult
nsCSSCounterStyleRule::SetDescriptor(nsCSSCounterDesc aDescID,
                                     const nsAString& aValue)
{
  nsCSSParser parser;
  nsCSSValue value;
  CSSStyleSheet* sheet = GetStyleSheet();
  nsIURI* baseURL = nullptr;
  nsIPrincipal* principal = nullptr;
  if (sheet) {
    baseURL = sheet->GetBaseURI();
    principal = sheet->Principal();
  }
  if (parser.ParseCounterDescriptor(aDescID, aValue, nullptr,
                                    baseURL, principal, value)) {
    if (CheckDescValue(GetSystem(), aDescID, value)) {
      SetDesc(aDescID, value);
    }
  }
  return NS_OK;
}

#define CSS_COUNTER_DESC_SETTER(name_)                        \
NS_IMETHODIMP                                                 \
nsCSSCounterStyleRule::Set##name_(const nsAString& a##name_)  \
{                                                             \
  return SetDescriptor(eCSSCounterDesc_##name_, a##name_);    \
}
CSS_COUNTER_DESC_SETTER(System)
CSS_COUNTER_DESC_SETTER(Symbols)
CSS_COUNTER_DESC_SETTER(AdditiveSymbols)
CSS_COUNTER_DESC_SETTER(Negative)
CSS_COUNTER_DESC_SETTER(Prefix)
CSS_COUNTER_DESC_SETTER(Suffix)
CSS_COUNTER_DESC_SETTER(Range)
CSS_COUNTER_DESC_SETTER(Pad)
CSS_COUNTER_DESC_SETTER(Fallback)
CSS_COUNTER_DESC_SETTER(SpeakAs)
#undef CSS_COUNTER_DESC_SETTER

 size_t
nsCSSCounterStyleRule::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this);
}
