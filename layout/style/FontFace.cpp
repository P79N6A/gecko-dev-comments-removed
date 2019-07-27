




#include "mozilla/dom/FontFace.h"

#include "mozilla/dom/FontFaceBinding.h"
#include "mozilla/dom/FontFaceSet.h"
#include "mozilla/dom/Promise.h"
#include "nsCSSParser.h"
#include "nsCSSRules.h"
#include "nsIDocument.h"
#include "nsStyleUtil.h"

using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(FontFace, mParent, mLoaded, mRule)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(FontFace)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(FontFace)
NS_IMPL_CYCLE_COLLECTING_RELEASE(FontFace)

FontFace::FontFace(nsISupports* aParent, nsPresContext* aPresContext)
  : mParent(aParent)
  , mPresContext(aPresContext)
  , mStatus(FontFaceLoadStatus::Unloaded)
{
  MOZ_COUNT_CTOR(FontFace);

  MOZ_ASSERT(mPresContext);

  SetIsDOMBinding();

  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(aParent);

  if (global) {
    ErrorResult rv;
    mLoaded = Promise::Create(global, rv);
  }
}

FontFace::~FontFace()
{
  MOZ_COUNT_DTOR(FontFace);

  SetUserFontEntry(nullptr);
}

JSObject*
FontFace::WrapObject(JSContext* aCx)
{
  return FontFaceBinding::Wrap(aCx, this);
}

static FontFaceLoadStatus
LoadStateToStatus(gfxUserFontEntry::UserFontLoadState aLoadState)
{
  switch (aLoadState) {
    case gfxUserFontEntry::UserFontLoadState::STATUS_NOT_LOADED:
      return FontFaceLoadStatus::Unloaded;
    case gfxUserFontEntry::UserFontLoadState::STATUS_LOADING:
      return FontFaceLoadStatus::Loading;
    case gfxUserFontEntry::UserFontLoadState::STATUS_LOADED:
      return FontFaceLoadStatus::Loaded;
    case gfxUserFontEntry::UserFontLoadState::STATUS_FAILED:
      return FontFaceLoadStatus::Error;
  }
  NS_NOTREACHED("invalid aLoadState value");
  return FontFaceLoadStatus::Error;
}

already_AddRefed<FontFace>
FontFace::CreateForRule(nsISupports* aGlobal,
                        nsPresContext* aPresContext,
                        nsCSSFontFaceRule* aRule,
                        gfxUserFontEntry* aUserFontEntry)
{
  nsCOMPtr<nsIGlobalObject> globalObject = do_QueryInterface(aGlobal);

  nsRefPtr<FontFace> obj = new FontFace(aGlobal, aPresContext);
  obj->mRule = aRule;
  obj->SetUserFontEntry(aUserFontEntry);
  return obj.forget();
}

already_AddRefed<FontFace>
FontFace::Constructor(const GlobalObject& aGlobal,
                      const nsAString& aFamily,
                      const StringOrArrayBufferOrArrayBufferView& aSource,
                      const FontFaceDescriptors& aDescriptors,
                      ErrorResult& aRv)
{
  nsISupports* global = aGlobal.GetAsSupports();
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(global);
  nsIDocument* doc = window->GetDoc();
  if (!doc) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsIPresShell* shell = doc->GetShell();
  if (!shell) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsPresContext* presContext = shell->GetPresContext();
  if (!presContext) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<FontFace> obj = new FontFace(global, presContext);
  return obj.forget();
}

void
FontFace::GetFamily(nsString& aResult)
{
  mPresContext->FlushUserFontSet();

  
  nsCSSValue value;
  GetDesc(eCSSFontDesc_Family, value);

  aResult.Truncate();
  nsDependentString family(value.GetStringBufferValue());
  nsStyleUtil::AppendEscapedCSSString(family, aResult);
}

void
FontFace::SetFamily(const nsAString& aValue, ErrorResult& aRv)
{
  mPresContext->FlushUserFontSet();
  SetDescriptor(eCSSFontDesc_Family, aValue, aRv);
}

void
FontFace::GetStyle(nsString& aResult)
{
  mPresContext->FlushUserFontSet();
  GetDesc(eCSSFontDesc_Style, eCSSProperty_font_style, aResult);
}

void
FontFace::SetStyle(const nsAString& aValue, ErrorResult& aRv)
{
  mPresContext->FlushUserFontSet();
  SetDescriptor(eCSSFontDesc_Style, aValue, aRv);
}

void
FontFace::GetWeight(nsString& aResult)
{
  mPresContext->FlushUserFontSet();
  GetDesc(eCSSFontDesc_Weight, eCSSProperty_font_weight, aResult);
}

void
FontFace::SetWeight(const nsAString& aValue, ErrorResult& aRv)
{
  mPresContext->FlushUserFontSet();
  SetDescriptor(eCSSFontDesc_Weight, aValue, aRv);
}

void
FontFace::GetStretch(nsString& aResult)
{
  mPresContext->FlushUserFontSet();
  GetDesc(eCSSFontDesc_Stretch, eCSSProperty_font_stretch, aResult);
}

void
FontFace::SetStretch(const nsAString& aValue, ErrorResult& aRv)
{
  mPresContext->FlushUserFontSet();
  SetDescriptor(eCSSFontDesc_Stretch, aValue, aRv);
}

void
FontFace::GetUnicodeRange(nsString& aResult)
{
  mPresContext->FlushUserFontSet();

  nsCSSValue value;
  GetDesc(eCSSFontDesc_UnicodeRange, value);

  aResult.Truncate();
  nsStyleUtil::AppendUnicodeRange(value, aResult);
}

void
FontFace::SetUnicodeRange(const nsAString& aValue, ErrorResult& aRv)
{
  mPresContext->FlushUserFontSet();
  SetDescriptor(eCSSFontDesc_UnicodeRange, aValue, aRv);
}

void
FontFace::GetVariant(nsString& aResult)
{
  mPresContext->FlushUserFontSet();

  
  
  aResult.AssignLiteral("normal");
}

void
FontFace::SetVariant(const nsAString& aValue, ErrorResult& aRv)
{
  mPresContext->FlushUserFontSet();

  
  
}

void
FontFace::GetFeatureSettings(nsString& aResult)
{
  mPresContext->FlushUserFontSet();

  nsCSSValue value;
  GetDesc(eCSSFontDesc_FontFeatureSettings, value);

  aResult.Truncate();
  nsStyleUtil::AppendFontFeatureSettings(value, aResult);
}

void
FontFace::SetFeatureSettings(const nsAString& aValue, ErrorResult& aRv)
{
  mPresContext->FlushUserFontSet();
  SetDescriptor(eCSSFontDesc_FontFeatureSettings, aValue, aRv);
}

FontFaceLoadStatus
FontFace::Status()
{
  return mStatus;
}

Promise*
FontFace::Load(ErrorResult& aRv)
{
  mPresContext->FlushUserFontSet();

  if (!mLoaded) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  if (mStatus != FontFaceLoadStatus::Unloaded) {
    return mLoaded;
  }

  SetStatus(FontFaceLoadStatus::Loading);

  mUserFontEntry->Load();
  return mLoaded;
}

Promise*
FontFace::GetLoaded(ErrorResult& aRv)
{
  mPresContext->FlushUserFontSet();

  if (!mLoaded) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  return mLoaded;
}

void
FontFace::SetStatus(FontFaceLoadStatus aStatus)
{
  if (mStatus == aStatus) {
    return;
  }

  if (aStatus < mStatus) {
    
    
    
    
    
    return;
  }

  mStatus = aStatus;

  if (!mLoaded) {
    return;
  }

  if (mStatus == FontFaceLoadStatus::Loaded) {
    mLoaded->MaybeResolve(this);
  } else if (mStatus == FontFaceLoadStatus::Error) {
    
    mLoaded->MaybeReject(NS_ERROR_DOM_NETWORK_ERR);
  }
}

bool
FontFace::ParseDescriptor(nsCSSFontDesc aDescID,
                          const nsAString& aString,
                          nsCSSValue& aResult)
{
  nsCSSParser parser;

  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(mParent);
  nsCOMPtr<nsIPrincipal> principal = global->PrincipalOrNull();

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(mParent);
  nsCOMPtr<nsIURI> base = window->GetDocBaseURI();

  if (!parser.ParseFontFaceDescriptor(aDescID, aString,
                                      nullptr, 
                                      base,
                                      principal,
                                      aResult)) {
    aResult.Reset();
    return false;
  }

  return true;
}

void
FontFace::SetDescriptor(nsCSSFontDesc aFontDesc,
                        const nsAString& aValue,
                        ErrorResult& aRv)
{
  NS_ASSERTION(!mRule, "we don't handle rule-connected FontFace objects yet");
  if (mRule) {
    return;
  }

  nsCSSValue parsedValue;
  if (!ParseDescriptor(aFontDesc, aValue, parsedValue)) {
    aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }

  mDescriptors->Get(aFontDesc) = parsedValue;

  
  
}

void
FontFace::GetDesc(nsCSSFontDesc aDescID, nsCSSValue& aResult) const
{
  if (mRule) {
    MOZ_ASSERT(!mDescriptors);
    mRule->GetDesc(aDescID, aResult);
  } else {
    aResult = mDescriptors->Get(aDescID);
  }
}

void
FontFace::GetDesc(nsCSSFontDesc aDescID,
                  nsCSSProperty aPropID,
                  nsString& aResult) const
{
  nsCSSValue value;
  GetDesc(aDescID, value);

  aResult.Truncate();

  
  if (value.GetUnit() == eCSSUnit_Null) {
    if (aDescID == eCSSFontDesc_UnicodeRange) {
      aResult.AssignLiteral("U+0-10FFFF");
    } else if (aDescID != eCSSFontDesc_Family &&
               aDescID != eCSSFontDesc_Src) {
      aResult.AssignLiteral("normal");
    }
  } else {
    value.AppendToString(aPropID, aResult, nsCSSValue::eNormalized);
  }
}

void
FontFace::SetUserFontEntry(gfxUserFontEntry* aEntry)
{
  if (mUserFontEntry) {
    mUserFontEntry->mFontFaces.RemoveElement(this);
  }

  mUserFontEntry = static_cast<Entry*>(aEntry);
  if (mUserFontEntry) {
    mUserFontEntry->mFontFaces.AppendElement(this);

    
    
    
    
    
    
    
    
    
    
    FontFaceLoadStatus newStatus =
      LoadStateToStatus(mUserFontEntry->LoadState());
    if (newStatus > mStatus) {
      SetStatus(newStatus);
    }
  }
}

bool
FontFace::GetFamilyName(nsString& aResult)
{
  nsCSSValue value;
  GetDesc(eCSSFontDesc_Family, value);

  if (value.GetUnit() == eCSSUnit_String) {
    nsString familyname;
    value.GetStringValue(familyname);
    aResult.Append(familyname);
  }

  return !aResult.IsEmpty();
}



 void
FontFace::Entry::SetLoadState(UserFontLoadState aLoadState)
{
  gfxUserFontEntry::SetLoadState(aLoadState);

  for (size_t i = 0; i < mFontFaces.Length(); i++) {
    mFontFaces[i]->SetStatus(LoadStateToStatus(aLoadState));
  }
}
