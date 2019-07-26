





#include "CounterStyleManager.h"

#include "mozilla/Types.h"
#include "mozilla/CheckedInt.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/ArrayUtils.h"
#include "nsString.h"
#include "nsStyleSet.h"
#include "nsCSSRules.h"
#include "nsTArray.h"
#include "nsBulletFrame.h"
#include "nsTHashtable.h"
#include "nsUnicodeProperties.h"




extern const char16_t gJapaneseNegative[];
extern const char16_t gKoreanNegative[];
extern const char16_t gSimpChineseNegative[];
extern const char16_t gTradChineseNegative[];

namespace mozilla {

struct AdditiveSymbol
{
  CounterValue weight;
  nsString symbol;
};

struct NegativeType
{
  nsString before, after;
};

struct PadType
{
  int32_t width;
  nsString symbol;
};









#define LENGTH_LIMIT 150

static bool
GetCyclicCounterText(CounterValue aOrdinal,
                     nsSubstring& aResult,
                     const nsTArray<nsString>& aSymbols)
{
  NS_ABORT_IF_FALSE(aSymbols.Length() >= 1,
                    "No symbol available for cyclic counter.");
  auto n = aSymbols.Length();
  CounterValue index = (aOrdinal - 1) % n;
  aResult = aSymbols[index >= 0 ? index : index + n];
  return true;
}

static bool
GetFixedCounterText(CounterValue aOrdinal,
                    nsSubstring& aResult,
                    CounterValue aStart,
                    const nsTArray<nsString>& aSymbols)
{
  CounterValue index = aOrdinal - aStart;
  if (index >= 0 && index < CounterValue(aSymbols.Length())) {
    aResult = aSymbols[index];
    return true;
  } else {
    return false;
  }
}

static bool
GetSymbolicCounterText(CounterValue aOrdinal,
                       nsSubstring& aResult,
                       const nsTArray<nsString>& aSymbols)
{
  NS_ABORT_IF_FALSE(aSymbols.Length() >= 1,
                    "No symbol available for symbolic counter.");
  NS_ABORT_IF_FALSE(aOrdinal >= 0, "Invalid ordinal.");
  if (aOrdinal == 0) {
    return false;
  }

  aResult.Truncate();
  auto n = aSymbols.Length();
  const nsString& symbol = aSymbols[(aOrdinal - 1) % n];
  size_t len = (aOrdinal + n - 1) / n;
  auto symbolLength = symbol.Length();
  if (symbolLength > 0) {
    if (len > LENGTH_LIMIT || symbolLength > LENGTH_LIMIT ||
        len * symbolLength > LENGTH_LIMIT) {
      return false;
    }
    for (size_t i = 0; i < len; ++i) {
      aResult.Append(symbol);
    }
  }
  return true;
}

static bool
GetAlphabeticCounterText(CounterValue aOrdinal,
                         nsSubstring& aResult,
                         const nsTArray<nsString>& aSymbols)
{
  NS_ABORT_IF_FALSE(aSymbols.Length() >= 2,
                    "Too few symbols for alphabetic counter.");
  NS_ABORT_IF_FALSE(aOrdinal >= 0, "Invalid ordinal.");
  if (aOrdinal == 0) {
    return false;
  }

  auto n = aSymbols.Length();
  
  
  
  nsAutoTArray<int32_t, std::numeric_limits<CounterValue>::digits> indexes;
  while (aOrdinal > 0) {
    --aOrdinal;
    indexes.AppendElement(aOrdinal % n);
    aOrdinal /= n;
  }

  aResult.Truncate();
  for (auto i = indexes.Length(); i > 0; --i) {
    aResult.Append(aSymbols[indexes[i - 1]]);
  }
  return true;
}

static bool
GetNumericCounterText(CounterValue aOrdinal,
                      nsSubstring& aResult,
                      const nsTArray<nsString>& aSymbols)
{
  NS_ABORT_IF_FALSE(aSymbols.Length() >= 2,
                    "Too few symbols for numeric counter.");
  NS_ABORT_IF_FALSE(aOrdinal >= 0, "Invalid ordinal.");

  if (aOrdinal == 0) {
    aResult = aSymbols[0];
    return true;
  }

  auto n = aSymbols.Length();
  nsAutoTArray<int32_t, std::numeric_limits<CounterValue>::digits> indexes;
  while (aOrdinal > 0) {
    indexes.AppendElement(aOrdinal % n);
    aOrdinal /= n;
  }

  aResult.Truncate();
  for (auto i = indexes.Length(); i > 0; --i) {
    aResult.Append(aSymbols[indexes[i - 1]]);
  }
  return true;
}

static bool
GetAdditiveCounterText(CounterValue aOrdinal,
                       nsSubstring& aResult,
                       const nsTArray<AdditiveSymbol>& aSymbols)
{
  NS_ABORT_IF_FALSE(aOrdinal >= 0, "Invalid ordinal.");

  if (aOrdinal == 0) {
    const AdditiveSymbol& last = aSymbols.LastElement();
    if (last.weight == 0) {
      aResult = last.symbol;
      return true;
    }
    return false;
  }

  aResult.Truncate();
  size_t length = 0;
  for (size_t i = 0, iEnd = aSymbols.Length(); i < iEnd; ++i) {
    const AdditiveSymbol& symbol = aSymbols[i];
    if (symbol.weight == 0) {
      break;
    }
    CounterValue times = aOrdinal / symbol.weight;
    if (times > 0) {
      auto symbolLength = symbol.symbol.Length();
      if (symbolLength > 0) {
        length += times * symbolLength;
        if (times > LENGTH_LIMIT ||
            symbolLength > LENGTH_LIMIT ||
            length > LENGTH_LIMIT) {
          return false;
        }
        for (CounterValue j = 0; j < times; ++j) {
          aResult.Append(symbol.symbol);
        }
      }
      aOrdinal -= times * symbol.weight;
    }
  }
  return aOrdinal == 0;
}

class BuiltinCounterStyle : public CounterStyle
{
public:
  friend class CounterStyleManager;

  
  MOZ_CONSTEXPR BuiltinCounterStyle()
    : CounterStyle(NS_STYLE_LIST_STYLE_NONE)
  {
  }

protected:
  MOZ_CONSTEXPR BuiltinCounterStyle(int32_t aStyle)
    : CounterStyle(aStyle)
  {
  }

public:
  virtual void GetPrefix(nsSubstring& aResult) MOZ_OVERRIDE;
  virtual void GetSuffix(nsSubstring& aResult) MOZ_OVERRIDE;
  virtual void GetSpokenCounterText(CounterValue aOrdinal,
                                    nsSubstring& aResult,
                                    bool& aIsBullet) MOZ_OVERRIDE;
  virtual bool IsBullet() MOZ_OVERRIDE;

  virtual void GetNegative(NegativeType& aResult) MOZ_OVERRIDE;
  virtual bool IsOrdinalInRange(CounterValue aOrdinal) MOZ_OVERRIDE;
  virtual bool IsOrdinalInAutoRange(CounterValue aOrdinal) MOZ_OVERRIDE;
  virtual void GetPad(PadType& aResult) MOZ_OVERRIDE;
  virtual CounterStyle* GetFallback() MOZ_OVERRIDE;
  virtual uint8_t GetSpeakAs() MOZ_OVERRIDE;
  virtual bool UseNegativeSign() MOZ_OVERRIDE;

  virtual void CallFallbackStyle(CounterValue aOrdinal,
                                 nsSubstring& aResult,
                                 bool& aIsRTL) MOZ_OVERRIDE;
  virtual bool GetInitialCounterText(CounterValue aOrdinal,
                                     nsSubstring& aResult,
                                     bool& aIsRTL) MOZ_OVERRIDE;

  
  NS_IMETHOD_(MozExternalRefCountType) AddRef() MOZ_OVERRIDE { return 2; }
  NS_IMETHOD_(MozExternalRefCountType) Release() MOZ_OVERRIDE { return 2; }
};

 void
BuiltinCounterStyle::GetPrefix(nsSubstring& aResult)
{
  aResult.Truncate();
}

 void
BuiltinCounterStyle::GetSuffix(nsSubstring& aResult)
{
  
  
  nsAutoString result;
  nsBulletFrame::GetListItemSuffix(mStyle, result);
  aResult = result;
}


static const char16_t kDiscCharacter = 0x2022;
static const char16_t kCircleCharacter = 0x25e6;
static const char16_t kSquareCharacter = 0x25aa;

 void
BuiltinCounterStyle::GetSpokenCounterText(CounterValue aOrdinal,
                                          nsSubstring& aResult,
                                          bool& aIsBullet)
{
  switch (mStyle) {
    case NS_STYLE_LIST_STYLE_NONE:
      aResult.Truncate();
      aIsBullet = true;
      break;
    case NS_STYLE_LIST_STYLE_DISC:
      aResult.Assign(kDiscCharacter);
      aIsBullet = true;
      break;
    case NS_STYLE_LIST_STYLE_CIRCLE:
      aResult.Assign(kCircleCharacter);
      aIsBullet = true;
      break;
    case NS_STYLE_LIST_STYLE_SQUARE:
      aResult.Assign(kSquareCharacter);
      aIsBullet = true;
      break;
    default:
      CounterStyle::GetSpokenCounterText(aOrdinal, aResult, aIsBullet);
      aIsBullet = false;
      break;
  }
}

 bool
BuiltinCounterStyle::IsBullet()
{
  switch (mStyle) {
    case NS_STYLE_LIST_STYLE_DISC:
    case NS_STYLE_LIST_STYLE_CIRCLE:
    case NS_STYLE_LIST_STYLE_SQUARE:
      return true;
    default:
      return false;
  }
}

 void
BuiltinCounterStyle::GetNegative(NegativeType& aResult)
{
  switch (mStyle) {
    case NS_STYLE_LIST_STYLE_JAPANESE_FORMAL:
    case NS_STYLE_LIST_STYLE_JAPANESE_INFORMAL:
      aResult.before = gJapaneseNegative;
      break;

    case NS_STYLE_LIST_STYLE_KOREAN_HANGUL_FORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_INFORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_FORMAL:
      aResult.before = gKoreanNegative;
      break;

    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_FORMAL:
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_INFORMAL:
      aResult.before = gSimpChineseNegative;
      break;

    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_FORMAL:
    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_INFORMAL:
      aResult.before = gTradChineseNegative;
      break;

    default:
      aResult.before.AssignLiteral(MOZ_UTF16("-"));
  }
  aResult.after.Truncate();
}

 bool
BuiltinCounterStyle::IsOrdinalInRange(CounterValue aOrdinal)
{
  switch (mStyle) {
    default:
    
    case NS_STYLE_LIST_STYLE_NONE:
    case NS_STYLE_LIST_STYLE_DISC:
    case NS_STYLE_LIST_STYLE_CIRCLE:
    case NS_STYLE_LIST_STYLE_SQUARE:
    
    case NS_STYLE_LIST_STYLE_DECIMAL:
    
    case NS_STYLE_LIST_STYLE_JAPANESE_FORMAL:
    case NS_STYLE_LIST_STYLE_JAPANESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_FORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_INFORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANGUL_FORMAL:
    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_FORMAL:
    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_FORMAL:
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_INFORMAL:
      return true;

    
    case NS_STYLE_LIST_STYLE_MOZ_ETHIOPIC_NUMERIC:
      return aOrdinal >= 1;

    
    case NS_STYLE_LIST_STYLE_HEBREW:
      return aOrdinal >= 1 && aOrdinal <= 999999;

    
    case NS_STYLE_LIST_STYLE_MOZ_TAMIL:
      return aOrdinal >= 1 && aOrdinal <= 9999;
  }
}

 bool
BuiltinCounterStyle::IsOrdinalInAutoRange(CounterValue aOrdinal)
{
  switch (mStyle) {
    
    case NS_STYLE_LIST_STYLE_NONE:
    case NS_STYLE_LIST_STYLE_DISC:
    case NS_STYLE_LIST_STYLE_CIRCLE:
    case NS_STYLE_LIST_STYLE_SQUARE:
    
    case NS_STYLE_LIST_STYLE_DECIMAL:
      return true;

    
    case NS_STYLE_LIST_STYLE_HEBREW:
      return aOrdinal >= 0;

    
    case NS_STYLE_LIST_STYLE_JAPANESE_FORMAL:
    case NS_STYLE_LIST_STYLE_JAPANESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_FORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_INFORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANGUL_FORMAL:
    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_FORMAL:
    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_FORMAL:
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_MOZ_ETHIOPIC_NUMERIC:
    case NS_STYLE_LIST_STYLE_MOZ_TAMIL:
      return IsOrdinalInRange(aOrdinal);

    default:
      NS_NOTREACHED("Unknown counter style");
      return false;
  }
}

 void
BuiltinCounterStyle::GetPad(PadType& aResult)
{
  aResult.width = 0;
  aResult.symbol.Truncate();
}

 CounterStyle*
BuiltinCounterStyle::GetFallback()
{
  
  
  return CounterStyleManager::GetDecimalStyle();
}

 uint8_t
BuiltinCounterStyle::GetSpeakAs()
{
  switch (mStyle) {
    case NS_STYLE_LIST_STYLE_NONE:
    case NS_STYLE_LIST_STYLE_DISC:
    case NS_STYLE_LIST_STYLE_CIRCLE:
    case NS_STYLE_LIST_STYLE_SQUARE:
      return NS_STYLE_COUNTER_SPEAKAS_BULLETS;
    default:
      return NS_STYLE_COUNTER_SPEAKAS_NUMBERS;
  }
}

 bool
BuiltinCounterStyle::UseNegativeSign()
{
  switch (mStyle) {
    case NS_STYLE_LIST_STYLE_NONE:
    case NS_STYLE_LIST_STYLE_DISC:
    case NS_STYLE_LIST_STYLE_CIRCLE:
    case NS_STYLE_LIST_STYLE_SQUARE:
      return false;
    default:
      return true;
  }
}

 void
BuiltinCounterStyle::CallFallbackStyle(CounterValue aOrdinal,
                                       nsSubstring& aResult,
                                       bool& aIsRTL)
{
  GetFallback()->GetCounterText(aOrdinal, aResult, aIsRTL);
}

 bool
BuiltinCounterStyle::GetInitialCounterText(CounterValue aOrdinal,
                                           nsSubstring& aResult,
                                           bool& aIsRTL)
{
  
  
  nsAutoString result;
  nsBulletFrame::AppendCounterText(mStyle, aOrdinal, result, aIsRTL);
  aResult = result;
  
  
  
  
  return true;
}

class DependentBuiltinCounterStyle MOZ_FINAL : public BuiltinCounterStyle
{
public:
  DependentBuiltinCounterStyle(int32_t aStyle, CounterStyleManager* aManager)
    : BuiltinCounterStyle(aStyle), 
      mManager(aManager)
  {
    NS_ASSERTION(IsDependentStyle(), "Not a dependent builtin style");
    NS_ABORT_IF_FALSE(!IsCustomStyle(), "Not a builtin style");
  }

  virtual CounterStyle* GetFallback() MOZ_OVERRIDE;

  
  
  NS_INLINE_DECL_REFCOUNTING(DependentBuiltinCounterStyle)

private:
  CounterStyleManager* mManager;
};

 CounterStyle*
DependentBuiltinCounterStyle::GetFallback()
{
  switch (GetStyle()) {
    case NS_STYLE_LIST_STYLE_JAPANESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_JAPANESE_FORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANGUL_FORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_INFORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_FORMAL:
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_FORMAL:
    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_FORMAL:
      
      
      
      
      return mManager->BuildCounterStyle(NS_LITERAL_STRING("cjk-decimal"));
    default:
      NS_NOTREACHED("Not a valid dependent builtin style");
      return BuiltinCounterStyle::GetFallback();
  }
}

class CustomCounterStyle MOZ_FINAL : public CounterStyle
{
public:
  CustomCounterStyle(CounterStyleManager* aManager,
                     nsCSSCounterStyleRule* aRule)
    : CounterStyle(NS_STYLE_LIST_STYLE_CUSTOM),
      mManager(aManager),
      mRule(aRule),
      mRuleGeneration(aRule->GetGeneration()),
      mSystem(aRule->GetSystem()),
      mFlags(0),
      mFallback(nullptr),
      mSpeakAsCounter(nullptr),
      mExtends(nullptr),
      mExtendsRoot(nullptr)
  {
  }

  
  
  
  void ResetCachedData();

  
  
  
  
  
  void ResetDependentData();

  nsCSSCounterStyleRule* GetRule() const { return mRule; }
  uint32_t GetRuleGeneration() const { return mRuleGeneration; }

  virtual void GetPrefix(nsSubstring& aResult) MOZ_OVERRIDE;
  virtual void GetSuffix(nsSubstring& aResult) MOZ_OVERRIDE;
  virtual void GetSpokenCounterText(CounterValue aOrdinal,
                                    nsSubstring& aResult,
                                    bool& aIsBullet) MOZ_OVERRIDE;
  virtual bool IsBullet() MOZ_OVERRIDE;

  virtual void GetNegative(NegativeType& aResult) MOZ_OVERRIDE;
  virtual bool IsOrdinalInRange(CounterValue aOrdinal) MOZ_OVERRIDE;
  virtual bool IsOrdinalInAutoRange(CounterValue aOrdinal) MOZ_OVERRIDE;
  virtual void GetPad(PadType& aResult) MOZ_OVERRIDE;
  virtual CounterStyle* GetFallback() MOZ_OVERRIDE;
  virtual uint8_t GetSpeakAs() MOZ_OVERRIDE;
  virtual bool UseNegativeSign() MOZ_OVERRIDE;

  virtual void CallFallbackStyle(CounterValue aOrdinal,
                                 nsSubstring& aResult,
                                 bool& aIsRTL) MOZ_OVERRIDE;
  virtual bool GetInitialCounterText(CounterValue aOrdinal,
                                     nsSubstring& aResult,
                                     bool& aIsRTL) MOZ_OVERRIDE;

  bool IsExtendsSystem()
  {
    return mSystem == NS_STYLE_COUNTER_SYSTEM_EXTENDS;
  }

  
  
  
  NS_INLINE_DECL_REFCOUNTING(CustomCounterStyle)

private:
  const nsTArray<nsString>& GetSymbols();
  const nsTArray<AdditiveSymbol>& GetAdditiveSymbols();

  
  
  
  
  
  
  
  uint8_t GetSpeakAsAutoValue();
  void ComputeRawSpeakAs(uint8_t& aSpeakAs,
                         CounterStyle*& aSpeakAsCounter);
  CounterStyle* ComputeSpeakAs();

  CounterStyle* ComputeExtends();
  CounterStyle* GetExtends();
  CounterStyle* GetExtendsRoot();

  
  
  
  CounterStyleManager* mManager;

  nsRefPtr<nsCSSCounterStyleRule> mRule;
  uint32_t mRuleGeneration;

  uint8_t mSystem;
  uint8_t mSpeakAs;

  enum {
    
    FLAG_EXTENDS_VISITED = 1 << 0,
    FLAG_EXTENDS_LOOP    = 1 << 1,
    FLAG_SPEAKAS_VISITED  = 1 << 2,
    FLAG_SPEAKAS_LOOP     = 1 << 3,
    
    FLAG_NEGATIVE_INITED  = 1 << 4,
    FLAG_PREFIX_INITED    = 1 << 5,
    FLAG_SUFFIX_INITED    = 1 << 6,
    FLAG_PAD_INITED       = 1 << 7,
    FLAG_SPEAKAS_INITED   = 1 << 8,
  };
  uint16_t mFlags;

  
  nsTArray<nsString> mSymbols;
  nsTArray<AdditiveSymbol> mAdditiveSymbols;
  NegativeType mNegative;
  nsString mPrefix, mSuffix;
  PadType mPad;

  
  
  
  
  
  
  
  
  

  CounterStyle* mFallback;
  
  
  CounterStyle* mSpeakAsCounter;

  CounterStyle* mExtends;
  
  
  
  CounterStyle* mExtendsRoot;
};

void
CustomCounterStyle::ResetCachedData()
{
  mSymbols.Clear();
  mAdditiveSymbols.Clear();
  mFlags &= ~(FLAG_NEGATIVE_INITED |
              FLAG_PREFIX_INITED |
              FLAG_SUFFIX_INITED |
              FLAG_PAD_INITED |
              FLAG_SPEAKAS_INITED);
  mFallback = nullptr;
  mSpeakAsCounter = nullptr;
  mExtends = nullptr;
  mExtendsRoot = nullptr;
  mRuleGeneration = mRule->GetGeneration();
}

void
CustomCounterStyle::ResetDependentData()
{
  mFlags &= ~FLAG_SPEAKAS_INITED;
  mSpeakAsCounter = nullptr;
  mFallback = nullptr;
  mExtends = nullptr;
  mExtendsRoot = nullptr;
  if (IsExtendsSystem()) {
    mFlags &= ~(FLAG_NEGATIVE_INITED |
                FLAG_PREFIX_INITED |
                FLAG_SUFFIX_INITED |
                FLAG_PAD_INITED);
  }
}

 void
CustomCounterStyle::GetPrefix(nsSubstring& aResult)
{
  if (!(mFlags & FLAG_PREFIX_INITED)) {
    mFlags |= FLAG_PREFIX_INITED;

    const nsCSSValue& value = mRule->GetDesc(eCSSCounterDesc_Prefix);
    if (value.UnitHasStringValue()) {
      value.GetStringValue(mPrefix);
    } else if (IsExtendsSystem()) {
      GetExtends()->GetPrefix(mPrefix);
    } else {
      mPrefix.Truncate();
    }
  }
  aResult = mPrefix;
}

 void
CustomCounterStyle::GetSuffix(nsSubstring& aResult)
{
  if (!(mFlags & FLAG_SUFFIX_INITED)) {
    mFlags |= FLAG_SUFFIX_INITED;

    const nsCSSValue& value = mRule->GetDesc(eCSSCounterDesc_Suffix);
    if (value.UnitHasStringValue()) {
      value.GetStringValue(mSuffix);
    } else if (IsExtendsSystem()) {
      GetExtends()->GetSuffix(mSuffix);
    } else {
      mSuffix.AssignLiteral(MOZ_UTF16(". "));
    }
  }
  aResult = mSuffix;
}

 void
CustomCounterStyle::GetSpokenCounterText(CounterValue aOrdinal,
                                         nsSubstring& aResult,
                                         bool& aIsBullet)
{
  if (GetSpeakAs() != NS_STYLE_COUNTER_SPEAKAS_OTHER) {
    CounterStyle::GetSpokenCounterText(aOrdinal, aResult, aIsBullet);
  } else {
    NS_ABORT_IF_FALSE(mSpeakAsCounter,
                      "mSpeakAsCounter should have been initialized.");
    mSpeakAsCounter->GetSpokenCounterText(aOrdinal, aResult, aIsBullet);
  }
}

 bool
CustomCounterStyle::IsBullet()
{
  switch (mSystem) {
    case NS_STYLE_COUNTER_SYSTEM_CYCLIC:
      
      return true;
    case NS_STYLE_COUNTER_SYSTEM_EXTENDS:
      return GetExtendsRoot()->IsBullet();
    default:
      return false;
  }
}

 void
CustomCounterStyle::GetNegative(NegativeType& aResult)
{
  if (!(mFlags & FLAG_NEGATIVE_INITED)) {
    mFlags |= FLAG_NEGATIVE_INITED;
    const nsCSSValue& value = mRule->GetDesc(eCSSCounterDesc_Negative);
    switch (value.GetUnit()) {
      case eCSSUnit_Ident:
      case eCSSUnit_String:
        value.GetStringValue(mNegative.before);
        mNegative.after.Truncate();
        break;
      case eCSSUnit_Pair: {
        const nsCSSValuePair& pair = value.GetPairValue();
        pair.mXValue.GetStringValue(mNegative.before);
        pair.mYValue.GetStringValue(mNegative.after);
        break;
      }
      default: {
        if (IsExtendsSystem()) {
          GetExtends()->GetNegative(mNegative);
        } else {
          mNegative.before.AssignLiteral(MOZ_UTF16("-"));
          mNegative.after.Truncate();
        }
      }
    }
  }
  aResult = mNegative;
}

static inline bool
IsRangeValueInfinite(const nsCSSValue& aValue)
{
  return aValue.GetUnit() == eCSSUnit_Enumerated &&
         aValue.GetIntValue() == NS_STYLE_COUNTER_RANGE_INFINITE;
}

 bool
CustomCounterStyle::IsOrdinalInRange(CounterValue aOrdinal)
{
  const nsCSSValue& value = mRule->GetDesc(eCSSCounterDesc_Range);
  if (value.GetUnit() == eCSSUnit_PairList) {
    for (const nsCSSValuePairList* item = value.GetPairListValue();
         item != nullptr; item = item->mNext) {
      const nsCSSValue& lowerBound = item->mXValue;
      const nsCSSValue& upperBound = item->mYValue;
      if ((IsRangeValueInfinite(lowerBound) ||
           aOrdinal >= lowerBound.GetIntValue()) &&
          (IsRangeValueInfinite(upperBound) ||
           aOrdinal <= upperBound.GetIntValue())) {
        return true;
      }
    }
    return false;
  } else if (IsExtendsSystem() && value.GetUnit() == eCSSUnit_None) {
    
    return GetExtends()->IsOrdinalInRange(aOrdinal);
  }
  return IsOrdinalInAutoRange(aOrdinal);
}

 bool
CustomCounterStyle::IsOrdinalInAutoRange(CounterValue aOrdinal)
{
  switch (mSystem) {
    case NS_STYLE_COUNTER_SYSTEM_CYCLIC:
    case NS_STYLE_COUNTER_SYSTEM_NUMERIC:
    case NS_STYLE_COUNTER_SYSTEM_FIXED:
      return true;
    case NS_STYLE_COUNTER_SYSTEM_ALPHABETIC:
    case NS_STYLE_COUNTER_SYSTEM_SYMBOLIC:
      return aOrdinal >= 1;
    case NS_STYLE_COUNTER_SYSTEM_ADDITIVE:
      return aOrdinal >= 0;
    case NS_STYLE_COUNTER_SYSTEM_EXTENDS:
      return GetExtendsRoot()->IsOrdinalInAutoRange(aOrdinal);
    default:
      NS_NOTREACHED("Invalid system for computing auto value.");
      return false;
  }
}

 void
CustomCounterStyle::GetPad(PadType& aResult)
{
  if (!(mFlags & FLAG_PAD_INITED)) {
    mFlags |= FLAG_PAD_INITED;
    const nsCSSValue& value = mRule->GetDesc(eCSSCounterDesc_Pad);
    if (value.GetUnit() == eCSSUnit_Pair) {
      const nsCSSValuePair& pair = value.GetPairValue();
      mPad.width = pair.mXValue.GetIntValue();
      pair.mYValue.GetStringValue(mPad.symbol);
    } else if (IsExtendsSystem()) {
      GetExtends()->GetPad(mPad);
    } else {
      mPad.width = 0;
      mPad.symbol.Truncate();
    }
  }
  aResult = mPad;
}

 CounterStyle*
CustomCounterStyle::GetFallback()
{
  if (!mFallback) {
    const nsCSSValue& value = mRule->GetDesc(eCSSCounterDesc_Fallback);
    if (value.UnitHasStringValue()) {
      mFallback = mManager->BuildCounterStyle(
          nsDependentString(value.GetStringBufferValue()));
    } else if (IsExtendsSystem()) {
      mFallback = GetExtends()->GetFallback();
    } else {
      mFallback = CounterStyleManager::GetDecimalStyle();
    }
  }
  return mFallback;
}

 uint8_t
CustomCounterStyle::GetSpeakAs()
{
  if (!(mFlags & FLAG_SPEAKAS_INITED)) {
    ComputeSpeakAs();
  }
  return mSpeakAs;
}

 bool
CustomCounterStyle::UseNegativeSign()
{
  switch (mSystem) {
    case NS_STYLE_COUNTER_SYSTEM_SYMBOLIC:
    case NS_STYLE_COUNTER_SYSTEM_ALPHABETIC:
    case NS_STYLE_COUNTER_SYSTEM_NUMERIC:
    case NS_STYLE_COUNTER_SYSTEM_ADDITIVE:
      return true;
    case NS_STYLE_COUNTER_SYSTEM_EXTENDS:
      return GetExtendsRoot()->UseNegativeSign();
    default:
      return false;
  }
}

 void
CustomCounterStyle::CallFallbackStyle(CounterValue aOrdinal,
                                      nsSubstring& aResult,
                                      bool& aIsRTL)
{
  CounterStyle* fallback = GetFallback();
  
  
  mFallback = CounterStyleManager::GetDecimalStyle();
  fallback->GetCounterText(aOrdinal, aResult, aIsRTL);
  mFallback = fallback;
}

 bool
CustomCounterStyle::GetInitialCounterText(CounterValue aOrdinal,
                                          nsSubstring& aResult,
                                          bool& aIsRTL)
{
  switch (mSystem) {
    case NS_STYLE_COUNTER_SYSTEM_CYCLIC:
      return GetCyclicCounterText(aOrdinal, aResult, GetSymbols());
    case NS_STYLE_COUNTER_SYSTEM_FIXED: {
      int32_t start = mRule->GetSystemArgument().GetIntValue();
      return GetFixedCounterText(aOrdinal, aResult, start, GetSymbols());
    }
    case NS_STYLE_COUNTER_SYSTEM_SYMBOLIC:
      return GetSymbolicCounterText(aOrdinal, aResult, GetSymbols());
    case NS_STYLE_COUNTER_SYSTEM_ALPHABETIC:
      return GetAlphabeticCounterText(aOrdinal, aResult, GetSymbols());
    case NS_STYLE_COUNTER_SYSTEM_NUMERIC:
      return GetNumericCounterText(aOrdinal, aResult, GetSymbols());
    case NS_STYLE_COUNTER_SYSTEM_ADDITIVE:
      return GetAdditiveCounterText(aOrdinal, aResult, GetAdditiveSymbols());
    case NS_STYLE_COUNTER_SYSTEM_EXTENDS:
      return GetExtendsRoot()->
        GetInitialCounterText(aOrdinal, aResult, aIsRTL);
    default:
      NS_NOTREACHED("Invalid system.");
      return false;
  }
}

const nsTArray<nsString>&
CustomCounterStyle::GetSymbols()
{
  if (mSymbols.IsEmpty()) {
    const nsCSSValue& values = mRule->GetDesc(eCSSCounterDesc_Symbols);
    for (const nsCSSValueList* item = values.GetListValue();
         item; item = item->mNext) {
      nsString* symbol = mSymbols.AppendElement();
      item->mValue.GetStringValue(*symbol);
    }
    mSymbols.Compact();
  }
  return mSymbols;
}

const nsTArray<AdditiveSymbol>&
CustomCounterStyle::GetAdditiveSymbols()
{
  if (mAdditiveSymbols.IsEmpty()) {
    const nsCSSValue& values = mRule->GetDesc(eCSSCounterDesc_AdditiveSymbols);
    for (const nsCSSValuePairList* item = values.GetPairListValue();
         item; item = item->mNext) {
      AdditiveSymbol* symbol = mAdditiveSymbols.AppendElement();
      symbol->weight = item->mXValue.GetIntValue();
      item->mYValue.GetStringValue(symbol->symbol);
    }
    mAdditiveSymbols.Compact();
  }
  return mAdditiveSymbols;
}


uint8_t
CustomCounterStyle::GetSpeakAsAutoValue()
{
  uint8_t system = mSystem;
  if (IsExtendsSystem()) {
    CounterStyle* root = GetExtendsRoot();
    if (!root->IsCustomStyle()) {
      
      return root->GetSpeakAs();
    }
    system = static_cast<CustomCounterStyle*>(root)->mSystem;
  }
  switch (system) {
    case NS_STYLE_COUNTER_SYSTEM_ALPHABETIC:
      return NS_STYLE_COUNTER_SPEAKAS_SPELL_OUT;
    case NS_STYLE_COUNTER_SYSTEM_CYCLIC:
      return NS_STYLE_COUNTER_SPEAKAS_BULLETS;
    default:
      return NS_STYLE_COUNTER_SPEAKAS_NUMBERS;
  }
}







void
CustomCounterStyle::ComputeRawSpeakAs(uint8_t& aSpeakAs,
                                    CounterStyle*& aSpeakAsCounter)
{
  NS_ASSERTION(!(mFlags & FLAG_SPEAKAS_INITED),
               "ComputeRawSpeakAs is called with speak-as inited.");

  const nsCSSValue& value = mRule->GetDesc(eCSSCounterDesc_SpeakAs);
  switch (value.GetUnit()) {
    case eCSSUnit_Auto:
      aSpeakAs = GetSpeakAsAutoValue();
      break;
    case eCSSUnit_Enumerated:
      aSpeakAs = value.GetIntValue();
      break;
    case eCSSUnit_Ident:
      aSpeakAs = NS_STYLE_COUNTER_SPEAKAS_OTHER;
      aSpeakAsCounter = mManager->BuildCounterStyle(
          nsDependentString(value.GetStringBufferValue()));
      break;
    case eCSSUnit_Null: {
      if (!IsExtendsSystem()) {
        aSpeakAs = GetSpeakAsAutoValue();
      } else {
        CounterStyle* extended = GetExtends();
        if (!extended->IsCustomStyle()) {
          
          aSpeakAs = extended->GetSpeakAs();
        } else {
          CustomCounterStyle* custom =
            static_cast<CustomCounterStyle*>(extended);
          if (!(custom->mFlags & FLAG_SPEAKAS_INITED)) {
            custom->ComputeRawSpeakAs(aSpeakAs, aSpeakAsCounter);
          } else {
            aSpeakAs = custom->mSpeakAs;
            aSpeakAsCounter = custom->mSpeakAsCounter;
          }
        }
      }
      break;
    }
    default:
      NS_NOTREACHED("Invalid speak-as value");
  }
}








CounterStyle*
CustomCounterStyle::ComputeSpeakAs()
{
  if (mFlags & FLAG_SPEAKAS_INITED) {
    if (mSpeakAs == NS_STYLE_COUNTER_SPEAKAS_OTHER) {
      return mSpeakAsCounter;
    }
    return this;
  }

  if (mFlags & FLAG_SPEAKAS_VISITED) {
    
    mFlags |= FLAG_SPEAKAS_LOOP;
    return nullptr;
  }

  CounterStyle* speakAsCounter;
  ComputeRawSpeakAs(mSpeakAs, speakAsCounter);

  bool inLoop = false;
  if (mSpeakAs != NS_STYLE_COUNTER_SPEAKAS_OTHER) {
    mSpeakAsCounter = nullptr;
  } else if (!speakAsCounter->IsCustomStyle()) {
    mSpeakAsCounter = speakAsCounter;
  } else {
    mFlags |= FLAG_SPEAKAS_VISITED;
    CounterStyle* target =
      static_cast<CustomCounterStyle*>(speakAsCounter)->ComputeSpeakAs();
    mFlags &= ~FLAG_SPEAKAS_VISITED;

    if (target) {
      NS_ASSERTION(!(mFlags & FLAG_SPEAKAS_LOOP),
                   "Invalid state for speak-as loop detecting");
      mSpeakAsCounter = target;
    } else {
      mSpeakAs = GetSpeakAsAutoValue();
      mSpeakAsCounter = nullptr;
      if (mFlags & FLAG_SPEAKAS_LOOP) {
        mFlags &= ~FLAG_SPEAKAS_LOOP;
      } else {
        inLoop = true;
      }
    }
  }

  mFlags |= FLAG_SPEAKAS_INITED;
  if (inLoop) {
    return nullptr;
  }
  return mSpeakAsCounter ? mSpeakAsCounter : this;
}








CounterStyle*
CustomCounterStyle::ComputeExtends()
{
  if (!IsExtendsSystem() || mExtends) {
    return this;
  }
  if (mFlags & FLAG_EXTENDS_VISITED) {
    
    mFlags |= FLAG_EXTENDS_LOOP;
    return nullptr;
  }

  const nsCSSValue& value = mRule->GetSystemArgument();
  CounterStyle* nextCounter = mManager->BuildCounterStyle(
      nsDependentString(value.GetStringBufferValue()));
  CounterStyle* target = nextCounter;
  if (nextCounter->IsCustomStyle()) {
    mFlags |= FLAG_EXTENDS_VISITED;
    target = static_cast<CustomCounterStyle*>(nextCounter)->ComputeExtends();
    mFlags &= ~FLAG_EXTENDS_VISITED;
  }

  if (target) {
    NS_ASSERTION(!(mFlags & FLAG_EXTENDS_LOOP),
                 "Invalid state for extends loop detecting");
    mExtends = nextCounter;
    return this;
  } else {
    mExtends = CounterStyleManager::GetDecimalStyle();
    if (mFlags & FLAG_EXTENDS_LOOP) {
      mFlags &= ~FLAG_EXTENDS_LOOP;
      return this;
    } else {
      return nullptr;
    }
  }
}

CounterStyle*
CustomCounterStyle::GetExtends()
{
  if (!mExtends) {
    
    ComputeExtends();
  }
  return mExtends;
}

CounterStyle*
CustomCounterStyle::GetExtendsRoot()
{
  if (!mExtendsRoot) {
    CounterStyle* extended = GetExtends();
    mExtendsRoot = extended;
    if (extended->IsCustomStyle()) {
      CustomCounterStyle* custom = static_cast<CustomCounterStyle*>(extended);
      if (custom->IsExtendsSystem()) {
        
        
        
        mExtendsRoot = custom->GetExtendsRoot();
      }
    }
  }
  return mExtendsRoot;
}

bool
CounterStyle::IsDependentStyle() const
{
  switch (mStyle) {
    
    case NS_STYLE_LIST_STYLE_CUSTOM:
    
    case NS_STYLE_LIST_STYLE_JAPANESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_JAPANESE_FORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANGUL_FORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_INFORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_FORMAL:
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_FORMAL:
    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_FORMAL:
      return true;

    
    default:
      return false;
  }
}

static int32_t
CountGraphemeClusters(const nsSubstring& aText)
{
  using mozilla::unicode::ClusterIterator;
  ClusterIterator iter(aText.Data(), aText.Length());
  int32_t result = 0;
  while (!iter.AtEnd()) {
    ++result;
    iter.Next();
  }
  return result;
}

void
CounterStyle::GetCounterText(CounterValue aOrdinal,
                             nsSubstring& aResult,
                             bool& aIsRTL)
{
  bool success = IsOrdinalInRange(aOrdinal);
  aIsRTL = false;

  if (success) {
    
    bool useNegativeSign = UseNegativeSign();
    nsAutoString initialText;
    CounterValue ordinal;
    if (!useNegativeSign) {
      ordinal = aOrdinal;
    } else {
      CheckedInt<CounterValue> absolute(Abs(aOrdinal));
      ordinal = absolute.isValid() ?
        absolute.value() : std::numeric_limits<CounterValue>::max();
    }
    success = GetInitialCounterText(ordinal, initialText, aIsRTL);

    
    if (success) {
      PadType pad;
      GetPad(pad);
      
      
      int32_t diff = pad.width - CountGraphemeClusters(initialText);
      aResult.Truncate();
      if (useNegativeSign && aOrdinal < 0) {
        NegativeType negative;
        GetNegative(negative);
        aResult.Append(negative.before);
        
        
        initialText.Append(negative.after);
      }
      if (diff > 0) {
        auto length = pad.symbol.Length();
        if (diff > LENGTH_LIMIT || length > LENGTH_LIMIT ||
            diff * length > LENGTH_LIMIT) {
          success = false;
        } else if (length > 0) {
          for (int32_t i = 0; i < diff; ++i) {
            aResult.Append(pad.symbol);
          }
        }
      }
      if (success) {
        aResult.Append(initialText);
      }
    }
  }

  if (!success) {
    CallFallbackStyle(aOrdinal, aResult, aIsRTL);
  }
}

 void
CounterStyle::GetSpokenCounterText(CounterValue aOrdinal,
                                   nsSubstring& aResult,
                                   bool& aIsBullet)
{
  bool isRTL; 
  aIsBullet = false;
  switch (GetSpeakAs()) {
    case NS_STYLE_COUNTER_SPEAKAS_BULLETS:
      aResult.Assign(kDiscCharacter);
      aIsBullet = true;
      break;
    case NS_STYLE_COUNTER_SPEAKAS_NUMBERS:
      
      
      CounterStyleManager::GetDecimalStyle()->
        GetCounterText(aOrdinal, aResult, isRTL);
      break;
    case NS_STYLE_COUNTER_SPEAKAS_SPELL_OUT:
      
      
    case NS_STYLE_COUNTER_SPEAKAS_WORDS:
      GetCounterText(aOrdinal, aResult, isRTL);
      break;
    case NS_STYLE_COUNTER_SPEAKAS_OTHER:
      
      NS_NOTREACHED("Invalid speak-as value");
      break;
    default:
      NS_NOTREACHED("Unknown speak-as value");
      break;
  }
}

static BuiltinCounterStyle gBuiltinStyleTable[NS_STYLE_LIST_STYLE__MAX];

CounterStyleManager::CounterStyleManager(nsPresContext* aPresContext)
  : mPresContext(aPresContext)
{
  
  mCacheTable.Put(NS_LITERAL_STRING("none"), GetNoneStyle());
  mCacheTable.Put(NS_LITERAL_STRING("decimal"), GetDecimalStyle());
}

CounterStyleManager::~CounterStyleManager()
{
  NS_ABORT_IF_FALSE(!mPresContext, "Disconnect should have been called");
}

 void
CounterStyleManager::InitializeBuiltinCounterStyles()
{
  for (uint32_t i = 0; i < NS_STYLE_LIST_STYLE__MAX; ++i) {
    gBuiltinStyleTable[i].mStyle = i;
  }
}

#ifdef DEBUG
static PLDHashOperator
CheckRefCount(const nsSubstring& aKey,
              CounterStyle* aStyle,
              void* aArg)
{
  aStyle->AddRef();
  auto refcnt = aStyle->Release();
  NS_ASSERTION(!aStyle->IsDependentStyle() || refcnt == 1,
               "Counter style is still referenced by other objects.");
  return PL_DHASH_NEXT;
}
#endif

void
CounterStyleManager::Disconnect()
{
#ifdef DEBUG
  mCacheTable.EnumerateRead(CheckRefCount, nullptr);
#endif
  mCacheTable.Clear();
  mPresContext = nullptr;
}

CounterStyle*
CounterStyleManager::BuildCounterStyle(const nsSubstring& aName)
{
  CounterStyle* data = mCacheTable.GetWeak(aName);
  if (data) {
    return data;
  }

  
  
  nsCSSCounterStyleRule* rule =
    mPresContext->StyleSet()->CounterStyleRuleForName(mPresContext, aName);
  if (rule) {
    data = new CustomCounterStyle(this, rule);
  } else {
    int32_t type;
    nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(aName);
    if (nsCSSProps::FindKeyword(keyword, nsCSSProps::kListStyleKTable, type)) {
      if (gBuiltinStyleTable[type].IsDependentStyle()) {
        data = new DependentBuiltinCounterStyle(type, this);
      } else {
        data = GetBuiltinStyle(type);
      }
    }
  }
  if (!data) {
    data = GetDecimalStyle();
  }
  mCacheTable.Put(aName, data);
  return data;
}

 CounterStyle*
CounterStyleManager::GetBuiltinStyle(int32_t aStyle)
{
  NS_ABORT_IF_FALSE(0 <= aStyle && aStyle < NS_STYLE_LIST_STYLE__MAX,
                    "Require a valid builtin style constant");
  NS_ABORT_IF_FALSE(!gBuiltinStyleTable[aStyle].IsDependentStyle(),
                    "Cannot get dependent builtin style");
  NS_ASSERTION(aStyle != NS_STYLE_LIST_STYLE_LOWER_ROMAN &&
               aStyle != NS_STYLE_LIST_STYLE_UPPER_ROMAN &&
               aStyle != NS_STYLE_LIST_STYLE_LOWER_ALPHA &&
               aStyle != NS_STYLE_LIST_STYLE_UPPER_ALPHA,
               "lower/upper-roman/alpha should be custom counter style");
  return &gBuiltinStyleTable[aStyle];
}

struct InvalidateOldStyleData
{
  explicit InvalidateOldStyleData(nsPresContext* aPresContext)
    : mPresContext(aPresContext),
      mChanged(false)
  {
  }

  nsPresContext* mPresContext;
  nsTArray<nsRefPtr<CounterStyle>> mToBeRemoved;
  bool mChanged;
};

static PLDHashOperator
InvalidateOldStyle(const nsSubstring& aKey,
                   nsRefPtr<CounterStyle>& aStyle,
                   void* aArg)
{
  InvalidateOldStyleData* data = static_cast<InvalidateOldStyleData*>(aArg);
  bool toBeUpdated = false;
  bool toBeRemoved = false;
  nsCSSCounterStyleRule* newRule = data->mPresContext->
    StyleSet()->CounterStyleRuleForName(data->mPresContext, aKey);
  if (!newRule) {
    if (aStyle->IsCustomStyle()) {
      toBeRemoved = true;
    }
  } else {
    if (!aStyle->IsCustomStyle()) {
      toBeRemoved = true;
    } else {
      CustomCounterStyle* style =
        static_cast<CustomCounterStyle*>(aStyle.get());
      if (style->GetRule() != newRule) {
        toBeRemoved = true;
      } else if (style->GetRuleGeneration() != newRule->GetGeneration()) {
        toBeUpdated = true;
        style->ResetCachedData();
      }
    }
  }
  data->mChanged = data->mChanged || toBeUpdated || toBeRemoved;
  if (toBeRemoved) {
    if (aStyle->IsDependentStyle()) {
      
      
      
      
      data->mToBeRemoved.AppendElement(aStyle);
    }
    return PL_DHASH_REMOVE;
  }
  return PL_DHASH_NEXT;
}

static PLDHashOperator
InvalidateDependentData(const nsSubstring& aKey,
                        CounterStyle* aStyle,
                        void* aArg)
{
  if (aStyle->IsCustomStyle()) {
    CustomCounterStyle* custom = static_cast<CustomCounterStyle*>(aStyle);
    custom->ResetDependentData();
  }
  
  
  return PL_DHASH_NEXT;
}

bool
CounterStyleManager::NotifyRuleChanged()
{
  InvalidateOldStyleData data(mPresContext);
  mCacheTable.Enumerate(InvalidateOldStyle, &data);
  if (data.mChanged) {
    mCacheTable.EnumerateRead(InvalidateDependentData, nullptr);
  }
  return data.mChanged;
}

} 
