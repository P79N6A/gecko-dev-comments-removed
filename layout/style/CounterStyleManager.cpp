





#include "CounterStyleManager.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/CheckedInt.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/Types.h"
#include "mozilla/WritingModes.h"
#include "nsCSSRules.h"
#include "nsString.h"
#include "nsStyleSet.h"
#include "nsTArray.h"
#include "nsTHashtable.h"
#include "nsUnicodeProperties.h"
#include "prprf.h"

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
  MOZ_ASSERT(aSymbols.Length() >= 1,
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
  MOZ_ASSERT(aSymbols.Length() >= 1,
             "No symbol available for symbolic counter.");
  MOZ_ASSERT(aOrdinal >= 0, "Invalid ordinal.");
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
  MOZ_ASSERT(aSymbols.Length() >= 2,
             "Too few symbols for alphabetic counter.");
  MOZ_ASSERT(aOrdinal >= 0, "Invalid ordinal.");
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
  MOZ_ASSERT(aSymbols.Length() >= 2,
             "Too few symbols for numeric counter.");
  MOZ_ASSERT(aOrdinal >= 0, "Invalid ordinal.");

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
  MOZ_ASSERT(aOrdinal >= 0, "Invalid ordinal.");

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

static bool
DecimalToText(CounterValue aOrdinal, nsSubstring& aResult)
{
  
  char cbuf[std::numeric_limits<CounterValue>::digits10 + 3];
  PR_snprintf(cbuf, sizeof(cbuf), "%ld", aOrdinal);
  aResult.AssignASCII(cbuf);
  return true;
}






#define NUM_BUF_SIZE 34 

enum CJKIdeographicLang {
  CHINESE, KOREAN, JAPANESE
};
struct CJKIdeographicData {
  char16_t digit[10];
  char16_t unit[3];
  char16_t unit10K[2];
  uint8_t lang;
  bool informal;
};
static const CJKIdeographicData gDataJapaneseInformal = {
  {                           
    0x3007, 0x4e00, 0x4e8c, 0x4e09, 0x56db,
    0x4e94, 0x516d, 0x4e03, 0x516b, 0x4e5d
  },
  { 0x5341, 0x767e, 0x5343 }, 
  { 0x4e07, 0x5104 },         
  JAPANESE,                   
  true                        
};
static const CJKIdeographicData gDataJapaneseFormal = {
  {                           
    0x96f6, 0x58f1, 0x5f10, 0x53c2, 0x56db,
    0x4f0d, 0x516d, 0x4e03, 0x516b, 0x4e5d
  },
  { 0x62fe, 0x767e, 0x9621 }, 
  { 0x842c, 0x5104 },         
  JAPANESE,                   
  false                       
};
static const CJKIdeographicData gDataKoreanHangulFormal = {
  {                           
    0xc601, 0xc77c, 0xc774, 0xc0bc, 0xc0ac,
    0xc624, 0xc721, 0xce60, 0xd314, 0xad6c
  },
  { 0xc2ed, 0xbc31, 0xcc9c }, 
  { 0xb9cc, 0xc5b5 },         
  KOREAN,                     
  false                       
};
static const CJKIdeographicData gDataKoreanHanjaInformal = {
  {                           
    0x96f6, 0x4e00, 0x4e8c, 0x4e09, 0x56db,
    0x4e94, 0x516d, 0x4e03, 0x516b, 0x4e5d
  },
  { 0x5341, 0x767e, 0x5343 }, 
  { 0x842c, 0x5104 },         
  KOREAN,                     
  true                        
};
static const CJKIdeographicData gDataKoreanHanjaFormal = {
  {                           
    0x96f6, 0x58f9, 0x8cb3, 0x53c3, 0x56db,
    0x4e94, 0x516d, 0x4e03, 0x516b, 0x4e5d
  },
  { 0x62fe, 0x767e, 0x4edf }, 
  { 0x842c, 0x5104 },         
  KOREAN,                     
  false                       
};
static const CJKIdeographicData gDataSimpChineseInformal = {
  {                           
    0x96f6, 0x4e00, 0x4e8c, 0x4e09, 0x56db,
    0x4e94, 0x516d, 0x4e03, 0x516b, 0x4e5d
  },
  { 0x5341, 0x767e, 0x5343 }, 
  { 0x4e07, 0x4ebf },         
  CHINESE,                    
  true                        
};
static const CJKIdeographicData gDataSimpChineseFormal = {
  {                           
    0x96f6, 0x58f9, 0x8d30, 0x53c1, 0x8086,
    0x4f0d, 0x9646, 0x67d2, 0x634c, 0x7396
  },
  { 0x62fe, 0x4f70, 0x4edf }, 
  { 0x4e07, 0x4ebf },         
  CHINESE,                    
  false                       
};
static const CJKIdeographicData gDataTradChineseInformal = {
  {                           
    0x96f6, 0x4e00, 0x4e8c, 0x4e09, 0x56db,
    0x4e94, 0x516d, 0x4e03, 0x516b, 0x4e5d
  },
  { 0x5341, 0x767e, 0x5343 }, 
  { 0x842c, 0x5104 },         
  CHINESE,                    
  true                        
};
static const CJKIdeographicData gDataTradChineseFormal = {
  {                           
    0x96f6, 0x58f9, 0x8cb3, 0x53c3, 0x8086,
    0x4f0d, 0x9678, 0x67d2, 0x634c, 0x7396
  },
  { 0x62fe, 0x4f70, 0x4edf }, 
  { 0x842c, 0x5104 },         
  CHINESE,                    
  false                       
};

static bool
CJKIdeographicToText(CounterValue aOrdinal, nsSubstring& aResult,
                     const CJKIdeographicData& data)
{
  NS_ASSERTION(aOrdinal >= 0, "Only accept non-negative ordinal");
  char16_t buf[NUM_BUF_SIZE];
  int32_t idx = NUM_BUF_SIZE;
  int32_t pos = 0;
  bool needZero = (aOrdinal == 0);
  int32_t unitidx = 0, unit10Kidx = 0;
  do {
    unitidx = pos % 4;
    if (unitidx == 0) {
      unit10Kidx = pos / 4;
    }
    auto cur = static_cast<MakeUnsigned<CounterValue>::Type>(aOrdinal) % 10;
    if (cur == 0) {
      if (needZero) {
        needZero = false;
        buf[--idx] = data.digit[0];
      }
    } else {
      if (data.lang == CHINESE) {
        needZero = true;
      }
      if (unit10Kidx != 0) {
        if (data.lang == KOREAN && idx != NUM_BUF_SIZE) {
          buf[--idx] = ' ';
        }
        buf[--idx] = data.unit10K[unit10Kidx - 1];
      }
      if (unitidx != 0) {
        buf[--idx] = data.unit[unitidx - 1];
      }
      if (cur != 1) {
        buf[--idx] = data.digit[cur];
      } else {
        bool needOne = true;
        if (data.informal) {
          switch (data.lang) {
            case CHINESE:
              if (unitidx == 1 &&
                  (aOrdinal == 1 || (pos > 4 && aOrdinal % 1000 == 1))) {
                needOne = false;
              }
              break;
            case JAPANESE:
              if (unitidx > 0 &&
                  (unitidx != 3 || (pos == 3 && aOrdinal == 1))) {
                needOne = false;
              }
              break;
            case KOREAN:
              if (unitidx > 0 || (pos == 4 && (aOrdinal % 1000) == 1)) {
                needOne = false;
              }
              break;
          }
        }
        if (needOne) {
          buf[--idx] = data.digit[1];
        }
      }
      unit10Kidx = 0;
    }
    aOrdinal /= 10;
    pos++;
  } while (aOrdinal > 0);
  aResult.Assign(buf + idx, NUM_BUF_SIZE - idx);
  return true;
}

#define HEBREW_GERESH       0x05F3
static const char16_t gHebrewDigit[22] = 
{
  
  0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8,
  
  0x05D9, 0x05DB, 0x05DC, 0x05DE, 0x05E0, 0x05E1, 0x05E2, 0x05E4, 0x05E6,
  
  0x05E7, 0x05E8, 0x05E9, 0x05EA
};

static bool
HebrewToText(CounterValue aOrdinal, nsSubstring& aResult)
{
  if (aOrdinal < 1 || aOrdinal > 999999) {
    return false;
  }

  bool outputSep = false;
  nsAutoString allText, thousandsGroup;
  do {
    thousandsGroup.Truncate();
    int32_t n3 = aOrdinal % 1000;
    
    for(int32_t n1 = 400; n1 > 0; )
    {
      if( n3 >= n1)
      {
        n3 -= n1;
        thousandsGroup.Append(gHebrewDigit[(n1/100)-1+18]);
      } else {
        n1 -= 100;
      } 
    } 

    
    int32_t n2;
    if( n3 >= 10 )
    {
      
      if(( 15 == n3 ) || (16 == n3)) {
        
        
        
        n2 = 9;
        thousandsGroup.Append(gHebrewDigit[ n2 - 1]);
      } else {
        n2 = n3 - (n3 % 10);
        thousandsGroup.Append(gHebrewDigit[(n2/10)-1+9]);
      } 
      n3 -= n2;
    } 
  
    
    if ( n3 > 0)
      thousandsGroup.Append(gHebrewDigit[n3-1]);
    if (outputSep) 
      thousandsGroup.Append((char16_t)HEBREW_GERESH);
    if (allText.IsEmpty())
      allText = thousandsGroup;
    else
      allText = thousandsGroup + allText;
    aOrdinal /= 1000;
    outputSep = true;
  } while (aOrdinal >= 1);

  aResult = allText;
  return true;
}






#define ETHIOPIC_ONE             0x1369
#define ETHIOPIC_TEN             0x1372
#define ETHIOPIC_HUNDRED         0x137B
#define ETHIOPIC_TEN_THOUSAND    0x137C

static bool
EthiopicToText(CounterValue aOrdinal, nsSubstring& aResult)
{
  if (aOrdinal < 1) {
    return false;
  }

  nsAutoString asciiNumberString;      
  DecimalToText(aOrdinal, asciiNumberString);
  uint8_t asciiStringLength = asciiNumberString.Length();

  
  
  
  
  
  if (asciiStringLength & 1) {
    asciiNumberString.Insert(NS_LITERAL_STRING("0"), 0);
  } else {
    asciiStringLength--;
  }

  aResult.Truncate();
  
  
  
  for (uint8_t indexFromLeft = 0, groupIndexFromRight = asciiStringLength >> 1;
       indexFromLeft <= asciiStringLength;
       indexFromLeft += 2, groupIndexFromRight--) {
    uint8_t tensValue  = asciiNumberString.CharAt(indexFromLeft) & 0x0F;
    uint8_t unitsValue = asciiNumberString.CharAt(indexFromLeft + 1) & 0x0F;
    uint8_t groupValue = tensValue * 10 + unitsValue;

    bool oddGroup = (groupIndexFromRight & 1);

    
    if (aOrdinal > 1 &&
        groupValue == 1 &&                  
        (oddGroup || indexFromLeft == 0)) { 
      unitsValue = 0;
    }

    
    if (tensValue) {
      
      aResult.Append((char16_t) (tensValue +  ETHIOPIC_TEN - 1));
    }
    if (unitsValue) {
      
      aResult.Append((char16_t) (unitsValue + ETHIOPIC_ONE - 1));
    }
    
    
    if (oddGroup) {
      if (groupValue) {
        aResult.Append((char16_t) ETHIOPIC_HUNDRED);
      }
    } else {
      if (groupIndexFromRight) {
        aResult.Append((char16_t) ETHIOPIC_TEN_THOUSAND);
      }
    }
  }
  return true;
}

static uint8_t
GetDefaultSpeakAsForSystem(uint8_t aSystem)
{
  MOZ_ASSERT(aSystem != NS_STYLE_COUNTER_SYSTEM_EXTENDS,
             "Extends system does not have static default speak-as");
  switch (aSystem) {
    case NS_STYLE_COUNTER_SYSTEM_ALPHABETIC:
      return NS_STYLE_COUNTER_SPEAKAS_SPELL_OUT;
    case NS_STYLE_COUNTER_SYSTEM_CYCLIC:
      return NS_STYLE_COUNTER_SPEAKAS_BULLETS;
    default:
      return NS_STYLE_COUNTER_SPEAKAS_NUMBERS;
  }
}

static bool
SystemUsesNegativeSign(uint8_t aSystem)
{
  MOZ_ASSERT(aSystem != NS_STYLE_COUNTER_SYSTEM_EXTENDS,
             "Cannot check this for extending style");
  switch (aSystem) {
    case NS_STYLE_COUNTER_SYSTEM_SYMBOLIC:
    case NS_STYLE_COUNTER_SYSTEM_ALPHABETIC:
    case NS_STYLE_COUNTER_SYSTEM_NUMERIC:
    case NS_STYLE_COUNTER_SYSTEM_ADDITIVE:
      return true;
    default:
      return false;
  }
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
  MOZ_CONSTEXPR explicit BuiltinCounterStyle(int32_t aStyle)
    : CounterStyle(aStyle)
  {
  }

public:
  virtual void GetPrefix(nsSubstring& aResult) override;
  virtual void GetSuffix(nsSubstring& aResult) override;
  virtual void GetSpokenCounterText(CounterValue aOrdinal,
                                    WritingMode aWritingMode,
                                    nsSubstring& aResult,
                                    bool& aIsBullet) override;
  virtual bool IsBullet() override;

  virtual void GetNegative(NegativeType& aResult) override;
  virtual bool IsOrdinalInRange(CounterValue aOrdinal) override;
  virtual bool IsOrdinalInAutoRange(CounterValue aOrdinal) override;
  virtual void GetPad(PadType& aResult) override;
  virtual CounterStyle* GetFallback() override;
  virtual uint8_t GetSpeakAs() override;
  virtual bool UseNegativeSign() override;

  virtual bool GetInitialCounterText(CounterValue aOrdinal,
                                     WritingMode aWritingMode,
                                     nsSubstring& aResult,
                                     bool& aIsRTL) override;

  
  NS_IMETHOD_(MozExternalRefCountType) AddRef() override { return 2; }
  NS_IMETHOD_(MozExternalRefCountType) Release() override { return 2; }
};

 void
BuiltinCounterStyle::GetPrefix(nsSubstring& aResult)
{
  aResult.Truncate();
}

 void
BuiltinCounterStyle::GetSuffix(nsSubstring& aResult)
{
  switch (mStyle) {
    case NS_STYLE_LIST_STYLE_NONE:
      aResult.Truncate();
      break;
 
    case NS_STYLE_LIST_STYLE_DISC:
    case NS_STYLE_LIST_STYLE_CIRCLE:
    case NS_STYLE_LIST_STYLE_SQUARE:
    case NS_STYLE_LIST_STYLE_DISCLOSURE_CLOSED:
    case NS_STYLE_LIST_STYLE_DISCLOSURE_OPEN:
    case NS_STYLE_LIST_STYLE_ETHIOPIC_NUMERIC:
      aResult = ' ';
      break;

    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_FORMAL:
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_FORMAL:
    case NS_STYLE_LIST_STYLE_JAPANESE_INFORMAL:
    case NS_STYLE_LIST_STYLE_JAPANESE_FORMAL:
      aResult = 0x3001;
      break;

    case NS_STYLE_LIST_STYLE_KOREAN_HANGUL_FORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_INFORMAL:
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_FORMAL:
      aResult.AssignLiteral(MOZ_UTF16(", "));
      break;

    default:
      aResult.AssignLiteral(MOZ_UTF16(". "));
      break;
  }
}

static const char16_t kDiscCharacter = 0x2022;
static const char16_t kCircleCharacter = 0x25e6;
static const char16_t kSquareCharacter = 0x25fe;
static const char16_t kRightPointingCharacter = 0x25b8;
static const char16_t kLeftPointingCharacter = 0x25c2;
static const char16_t kDownPointingCharacter = 0x25be;

 void
BuiltinCounterStyle::GetSpokenCounterText(CounterValue aOrdinal,
                                          WritingMode aWritingMode,
                                          nsSubstring& aResult,
                                          bool& aIsBullet)
{
  switch (mStyle) {
    case NS_STYLE_LIST_STYLE_NONE:
    case NS_STYLE_LIST_STYLE_DISC:
    case NS_STYLE_LIST_STYLE_CIRCLE:
    case NS_STYLE_LIST_STYLE_SQUARE:
    case NS_STYLE_LIST_STYLE_DISCLOSURE_CLOSED:
    case NS_STYLE_LIST_STYLE_DISCLOSURE_OPEN: {
      
      bool isRTL;
      GetInitialCounterText(aOrdinal, aWritingMode, aResult, isRTL);
      aIsBullet = true;
      break;
    }
    default:
      CounterStyle::GetSpokenCounterText(
          aOrdinal, aWritingMode, aResult, aIsBullet);
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
    case NS_STYLE_LIST_STYLE_DISCLOSURE_CLOSED:
    case NS_STYLE_LIST_STYLE_DISCLOSURE_OPEN:
      return true;
    default:
      return false;
  }
}

static const char16_t gJapaneseNegative[] = {
  0x30de, 0x30a4, 0x30ca, 0x30b9, 0x0000
};
static const char16_t gKoreanNegative[] = {
  0xb9c8, 0xc774, 0xb108, 0xc2a4, 0x0020, 0x0000
};
static const char16_t gSimpChineseNegative[] = {
  0x8d1f, 0x0000
};
static const char16_t gTradChineseNegative[] = {
  0x8ca0, 0x0000
};

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
    case NS_STYLE_LIST_STYLE_DISCLOSURE_CLOSED:
    case NS_STYLE_LIST_STYLE_DISCLOSURE_OPEN:
    
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

    
    case NS_STYLE_LIST_STYLE_ETHIOPIC_NUMERIC:
      return aOrdinal >= 1;

    
    case NS_STYLE_LIST_STYLE_HEBREW:
      return aOrdinal >= 1 && aOrdinal <= 999999;
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
    case NS_STYLE_LIST_STYLE_DISCLOSURE_CLOSED:
    case NS_STYLE_LIST_STYLE_DISCLOSURE_OPEN:
    
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
    case NS_STYLE_LIST_STYLE_ETHIOPIC_NUMERIC:
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
    case NS_STYLE_LIST_STYLE_DISCLOSURE_CLOSED:
    case NS_STYLE_LIST_STYLE_DISCLOSURE_OPEN:
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
    case NS_STYLE_LIST_STYLE_DISCLOSURE_CLOSED:
    case NS_STYLE_LIST_STYLE_DISCLOSURE_OPEN:
      return false;
    default:
      return true;
  }
}

 bool
BuiltinCounterStyle::GetInitialCounterText(CounterValue aOrdinal,
                                           WritingMode aWritingMode,
                                           nsSubstring& aResult,
                                           bool& aIsRTL)
{
  aIsRTL = false;
  switch (mStyle) {
    
    
    case NS_STYLE_LIST_STYLE_NONE:
      aResult.Truncate();
      return true;
    case NS_STYLE_LIST_STYLE_DISC:
      aResult.Assign(kDiscCharacter);
      return true;
    case NS_STYLE_LIST_STYLE_CIRCLE:
      aResult.Assign(kCircleCharacter);
      return true;
    case NS_STYLE_LIST_STYLE_SQUARE:
      aResult.Assign(kSquareCharacter);
      return true;
    case NS_STYLE_LIST_STYLE_DISCLOSURE_CLOSED:
      if (aWritingMode.IsVertical()) {
        aResult.Assign(kDownPointingCharacter);
      } else if (aWritingMode.IsBidiLTR()) {
        aResult.Assign(kRightPointingCharacter);
      } else {
        aResult.Assign(kLeftPointingCharacter);
      }
      return true;
    case NS_STYLE_LIST_STYLE_DISCLOSURE_OPEN:
      if (!aWritingMode.IsVertical()) {
        aResult.Assign(kDownPointingCharacter);
      } else if (aWritingMode.IsVerticalLR()) {
        aResult.Assign(kRightPointingCharacter);
      } else {
        aResult.Assign(kLeftPointingCharacter);
      }
      return true;

    case NS_STYLE_LIST_STYLE_DECIMAL:
      return DecimalToText(aOrdinal, aResult);

    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_INFORMAL:
      return CJKIdeographicToText(aOrdinal, aResult, gDataTradChineseInformal);
    case NS_STYLE_LIST_STYLE_TRAD_CHINESE_FORMAL:
      return CJKIdeographicToText(aOrdinal, aResult, gDataTradChineseFormal);
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_INFORMAL:
      return CJKIdeographicToText(aOrdinal, aResult, gDataSimpChineseInformal);
    case NS_STYLE_LIST_STYLE_SIMP_CHINESE_FORMAL:
      return CJKIdeographicToText(aOrdinal, aResult, gDataSimpChineseFormal);
    case NS_STYLE_LIST_STYLE_JAPANESE_INFORMAL:
      return CJKIdeographicToText(aOrdinal, aResult, gDataJapaneseInformal);
    case NS_STYLE_LIST_STYLE_JAPANESE_FORMAL:
      return CJKIdeographicToText(aOrdinal, aResult, gDataJapaneseFormal);
    case NS_STYLE_LIST_STYLE_KOREAN_HANGUL_FORMAL:
      return CJKIdeographicToText(aOrdinal, aResult, gDataKoreanHangulFormal);
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_INFORMAL:
      return CJKIdeographicToText(aOrdinal, aResult, gDataKoreanHanjaInformal);
    case NS_STYLE_LIST_STYLE_KOREAN_HANJA_FORMAL:
      return CJKIdeographicToText(aOrdinal, aResult, gDataKoreanHanjaFormal);

    case NS_STYLE_LIST_STYLE_HEBREW: 
      aIsRTL = true;
      return HebrewToText(aOrdinal, aResult);
 
    case NS_STYLE_LIST_STYLE_ETHIOPIC_NUMERIC:
      return EthiopicToText(aOrdinal, aResult);

    default:
      NS_NOTREACHED("Unknown builtin counter style");
      return false;
  }
}

class DependentBuiltinCounterStyle final : public BuiltinCounterStyle
{
private:
  ~DependentBuiltinCounterStyle() {}
public:
  DependentBuiltinCounterStyle(int32_t aStyle, CounterStyleManager* aManager)
    : BuiltinCounterStyle(aStyle), 
      mManager(aManager)
  {
    NS_ASSERTION(IsDependentStyle(), "Not a dependent builtin style");
    MOZ_ASSERT(!IsCustomStyle(), "Not a builtin style");
  }

  virtual CounterStyle* GetFallback() override;

  
  
  NS_IMETHOD_(MozExternalRefCountType) AddRef() override;
  NS_IMETHOD_(MozExternalRefCountType) Release() override;

  void* operator new(size_t sz, nsPresContext* aPresContext) CPP_THROW_NEW
  {
    return aPresContext->PresShell()->AllocateByObjectID(
        nsPresArena::DependentBuiltinCounterStyle_id, sz);
  }

private:
  void Destroy()
  {
    nsIPresShell* shell = mManager->PresContext()->PresShell();
    this->~DependentBuiltinCounterStyle();
    shell->FreeByObjectID(nsPresArena::DependentBuiltinCounterStyle_id, this);
  }

  CounterStyleManager* mManager;

  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD
};

NS_IMPL_ADDREF(DependentBuiltinCounterStyle)
NS_IMPL_RELEASE_WITH_DESTROY(DependentBuiltinCounterStyle, Destroy())

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

class CustomCounterStyle final : public CounterStyle
{
private:
  ~CustomCounterStyle() {}
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

  virtual void GetPrefix(nsSubstring& aResult) override;
  virtual void GetSuffix(nsSubstring& aResult) override;
  virtual void GetSpokenCounterText(CounterValue aOrdinal,
                                    WritingMode aWritingMode,
                                    nsSubstring& aResult,
                                    bool& aIsBullet) override;
  virtual bool IsBullet() override;

  virtual void GetNegative(NegativeType& aResult) override;
  virtual bool IsOrdinalInRange(CounterValue aOrdinal) override;
  virtual bool IsOrdinalInAutoRange(CounterValue aOrdinal) override;
  virtual void GetPad(PadType& aResult) override;
  virtual CounterStyle* GetFallback() override;
  virtual uint8_t GetSpeakAs() override;
  virtual bool UseNegativeSign() override;

  virtual void CallFallbackStyle(CounterValue aOrdinal,
                                 WritingMode aWritingMode,
                                 nsSubstring& aResult,
                                 bool& aIsRTL) override;
  virtual bool GetInitialCounterText(CounterValue aOrdinal,
                                     WritingMode aWritingMode,
                                     nsSubstring& aResult,
                                     bool& aIsRTL) override;

  bool IsExtendsSystem()
  {
    return mSystem == NS_STYLE_COUNTER_SYSTEM_EXTENDS;
  }

  
  
  
  NS_IMETHOD_(MozExternalRefCountType) AddRef() override;
  NS_IMETHOD_(MozExternalRefCountType) Release() override;

  void* operator new(size_t sz, nsPresContext* aPresContext) CPP_THROW_NEW
  {
    return aPresContext->PresShell()->AllocateByObjectID(
        nsPresArena::CustomCounterStyle_id, sz);
  }

private:
  void Destroy()
  {
    nsIPresShell* shell = mManager->PresContext()->PresShell();
    this->~CustomCounterStyle();
    shell->FreeByObjectID(nsPresArena::CustomCounterStyle_id, this);
  }

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

  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD
};

NS_IMPL_ADDREF(CustomCounterStyle)
NS_IMPL_RELEASE_WITH_DESTROY(CustomCounterStyle, Destroy())

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
                                         WritingMode aWritingMode,
                                         nsSubstring& aResult,
                                         bool& aIsBullet)
{
  if (GetSpeakAs() != NS_STYLE_COUNTER_SPEAKAS_OTHER) {
    CounterStyle::GetSpokenCounterText(
        aOrdinal, aWritingMode, aResult, aIsBullet);
  } else {
    MOZ_ASSERT(mSpeakAsCounter,
               "mSpeakAsCounter should have been initialized.");
    mSpeakAsCounter->GetSpokenCounterText(
        aOrdinal, aWritingMode, aResult, aIsBullet);
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
  if (mSystem == NS_STYLE_COUNTER_SYSTEM_EXTENDS) {
    return GetExtendsRoot()->UseNegativeSign();
  }
  return SystemUsesNegativeSign(mSystem);
}

 void
CustomCounterStyle::CallFallbackStyle(CounterValue aOrdinal,
                                      WritingMode aWritingMode,
                                      nsSubstring& aResult,
                                      bool& aIsRTL)
{
  CounterStyle* fallback = GetFallback();
  
  
  mFallback = CounterStyleManager::GetDecimalStyle();
  fallback->GetCounterText(aOrdinal, aWritingMode, aResult, aIsRTL);
  mFallback = fallback;
}

 bool
CustomCounterStyle::GetInitialCounterText(CounterValue aOrdinal,
                                          WritingMode aWritingMode,
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
        GetInitialCounterText(aOrdinal, aWritingMode, aResult, aIsRTL);
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
  return GetDefaultSpeakAsForSystem(system);
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

AnonymousCounterStyle::AnonymousCounterStyle(const nsSubstring& aContent)
  : CounterStyle(NS_STYLE_LIST_STYLE_CUSTOM)
  , mSingleString(true)
  , mSystem(NS_STYLE_COUNTER_SYSTEM_CYCLIC)
{
  mSymbols.SetCapacity(1);
  mSymbols.AppendElement(aContent);
}

AnonymousCounterStyle::AnonymousCounterStyle(const nsCSSValue::Array* aParams)
  : CounterStyle(NS_STYLE_LIST_STYLE_CUSTOM)
  , mSingleString(false)
  , mSystem(aParams->Item(0).GetIntValue())
{
  for (const nsCSSValueList* item = aParams->Item(1).GetListValue();
       item; item = item->mNext) {
    item->mValue.GetStringValue(*mSymbols.AppendElement());
  }
  mSymbols.Compact();
}

 void
AnonymousCounterStyle::GetPrefix(nsAString& aResult)
{
  aResult.Truncate();
}

 void
AnonymousCounterStyle::GetSuffix(nsAString& aResult)
{
  if (IsSingleString()) {
    aResult.Truncate();
  } else {
    aResult = ' ';
  }
}

 bool
AnonymousCounterStyle::IsBullet()
{
  switch (mSystem) {
    case NS_STYLE_COUNTER_SYSTEM_CYCLIC:
      
      return true;
    default:
      return false;
  }
}

 void
AnonymousCounterStyle::GetNegative(NegativeType& aResult)
{
  aResult.before.AssignLiteral(MOZ_UTF16("-"));
  aResult.after.Truncate();
}

 bool
AnonymousCounterStyle::IsOrdinalInRange(CounterValue aOrdinal)
{
  switch (mSystem) {
    case NS_STYLE_COUNTER_SYSTEM_CYCLIC:
    case NS_STYLE_COUNTER_SYSTEM_NUMERIC:
    case NS_STYLE_COUNTER_SYSTEM_FIXED:
      return true;
    case NS_STYLE_COUNTER_SYSTEM_ALPHABETIC:
    case NS_STYLE_COUNTER_SYSTEM_SYMBOLIC:
      return aOrdinal >= 1;
    default:
      NS_NOTREACHED("Invalid system.");
      return false;
  }
}

 bool
AnonymousCounterStyle::IsOrdinalInAutoRange(CounterValue aOrdinal)
{
  return AnonymousCounterStyle::IsOrdinalInRange(aOrdinal);
}

 void
AnonymousCounterStyle::GetPad(PadType& aResult)
{
  aResult.width = 0;
  aResult.symbol.Truncate();
}

 CounterStyle*
AnonymousCounterStyle::GetFallback()
{
  return CounterStyleManager::GetDecimalStyle();
}

 uint8_t
AnonymousCounterStyle::GetSpeakAs()
{
  return GetDefaultSpeakAsForSystem(mSystem);
}

 bool
AnonymousCounterStyle::UseNegativeSign()
{
  return SystemUsesNegativeSign(mSystem);
}

 bool
AnonymousCounterStyle::GetInitialCounterText(CounterValue aOrdinal,
                                             WritingMode aWritingMode,
                                             nsAString& aResult,
                                             bool& aIsRTL)
{
  switch (mSystem) {
    case NS_STYLE_COUNTER_SYSTEM_CYCLIC:
      return GetCyclicCounterText(aOrdinal, aResult, mSymbols);
    case NS_STYLE_COUNTER_SYSTEM_FIXED:
      return GetFixedCounterText(aOrdinal, aResult, 1, mSymbols);
    case NS_STYLE_COUNTER_SYSTEM_SYMBOLIC:
      return GetSymbolicCounterText(aOrdinal, aResult, mSymbols);
    case NS_STYLE_COUNTER_SYSTEM_ALPHABETIC:
      return GetAlphabeticCounterText(aOrdinal, aResult, mSymbols);
    case NS_STYLE_COUNTER_SYSTEM_NUMERIC:
      return GetNumericCounterText(aOrdinal, aResult, mSymbols);
    default:
      NS_NOTREACHED("Invalid system.");
      return false;
  }
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
                             WritingMode aWritingMode,
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
    success = GetInitialCounterText(
        ordinal, aWritingMode, initialText, aIsRTL);

    
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
    CallFallbackStyle(aOrdinal, aWritingMode, aResult, aIsRTL);
  }
}

 void
CounterStyle::GetSpokenCounterText(CounterValue aOrdinal,
                                   WritingMode aWritingMode,
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
      DecimalToText(aOrdinal, aResult);
      break;
    case NS_STYLE_COUNTER_SPEAKAS_SPELL_OUT:
      
      
    case NS_STYLE_COUNTER_SPEAKAS_WORDS:
      GetCounterText(aOrdinal, WritingMode(), aResult, isRTL);
      break;
    case NS_STYLE_COUNTER_SPEAKAS_OTHER:
      
      NS_NOTREACHED("Invalid speak-as value");
      break;
    default:
      NS_NOTREACHED("Unknown speak-as value");
      break;
  }
}

 void
CounterStyle::CallFallbackStyle(CounterValue aOrdinal,
                                WritingMode aWritingMode,
                                nsAString& aResult,
                                bool& aIsRTL)
{
  GetFallback()->GetCounterText(aOrdinal, aWritingMode, aResult, aIsRTL);
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
  MOZ_ASSERT(!mPresContext, "Disconnect should have been called");
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
    mPresContext->StyleSet()->CounterStyleRuleForName(aName);
  if (rule) {
    data = new (mPresContext) CustomCounterStyle(this, rule);
  } else {
    int32_t type;
    nsCSSKeyword keyword = nsCSSKeywords::LookupKeyword(aName);
    if (nsCSSProps::FindKeyword(keyword, nsCSSProps::kListStyleKTable, type)) {
      if (gBuiltinStyleTable[type].IsDependentStyle()) {
        data = new (mPresContext) DependentBuiltinCounterStyle(type, this);
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
  MOZ_ASSERT(0 <= aStyle && aStyle < NS_STYLE_LIST_STYLE__MAX,
             "Require a valid builtin style constant");
  MOZ_ASSERT(!gBuiltinStyleTable[aStyle].IsDependentStyle(),
             "Cannot get dependent builtin style");
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
    StyleSet()->CounterStyleRuleForName(aKey);
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
      if (aStyle->IsCustomStyle()) {
        
        
        
        
        static_cast<CustomCounterStyle*>(aStyle.get())->ResetDependentData();
      }
      
      
      
      
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
