




#ifndef mozilla_CounterStyleManager_h_
#define mozilla_CounterStyleManager_h_

#include "nsStringFwd.h"
#include "nsRefPtrHashtable.h"
#include "nsHashKeys.h"

#include "nsStyleConsts.h"

#include "mozilla/Attributes.h"

#include "nsCSSValue.h"

class nsPresContext;

namespace mozilla {

class WritingMode;

typedef int32_t CounterValue;

class CounterStyleManager;
class AnonymousCounterStyle;

struct NegativeType;
struct PadType;

class CounterStyle
{
protected:
  explicit MOZ_CONSTEXPR CounterStyle(int32_t aStyle)
    : mStyle(aStyle)
  {
  }

private:
  CounterStyle(const CounterStyle& aOther) = delete;
  void operator=(const CounterStyle& other) = delete;

public:
  int32_t GetStyle() const { return mStyle; }
  bool IsNone() const { return mStyle == NS_STYLE_LIST_STYLE_NONE; }
  bool IsCustomStyle() const { return mStyle == NS_STYLE_LIST_STYLE_CUSTOM; }
  
  
  
  bool IsDependentStyle() const;

  virtual void GetPrefix(nsSubstring& aResult) = 0;
  virtual void GetSuffix(nsSubstring& aResult) = 0;
  void GetCounterText(CounterValue aOrdinal,
                      WritingMode aWritingMode,
                      nsSubstring& aResult,
                      bool& aIsRTL);
  virtual void GetSpokenCounterText(CounterValue aOrdinal,
                                    WritingMode aWritingMode,
                                    nsSubstring& aResult,
                                    bool& aIsBullet);

  
  
  virtual bool IsBullet() = 0;

  virtual void GetNegative(NegativeType& aResult) = 0;
  




  virtual bool IsOrdinalInRange(CounterValue aOrdinal) = 0;
  




  virtual bool IsOrdinalInAutoRange(CounterValue aOrdinal) = 0;
  virtual void GetPad(PadType& aResult) = 0;
  virtual CounterStyle* GetFallback() = 0;
  virtual uint8_t GetSpeakAs() = 0;
  virtual bool UseNegativeSign() = 0;

  virtual void CallFallbackStyle(CounterValue aOrdinal,
                                 WritingMode aWritingMode,
                                 nsSubstring& aResult,
                                 bool& aIsRTL);
  virtual bool GetInitialCounterText(CounterValue aOrdinal,
                                     WritingMode aWritingMode,
                                     nsSubstring& aResult,
                                     bool& aIsRTL) = 0;

  virtual AnonymousCounterStyle* AsAnonymous() { return nullptr; }

  NS_IMETHOD_(MozExternalRefCountType) AddRef() = 0;
  NS_IMETHOD_(MozExternalRefCountType) Release() = 0;

protected:
  int32_t mStyle;
};

class AnonymousCounterStyle final : public CounterStyle
{
public:
  explicit AnonymousCounterStyle(const nsSubstring& aContent);
  explicit AnonymousCounterStyle(const nsCSSValue::Array* aValue);

  virtual void GetPrefix(nsAString& aResult) override;
  virtual void GetSuffix(nsAString& aResult) override;
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

  virtual AnonymousCounterStyle* AsAnonymous() override { return this; }

  bool IsSingleString() const { return mSingleString; }
  uint8_t GetSystem() const { return mSystem; }
  const nsTArray<nsString>& GetSymbols() const { return mSymbols; }

  NS_INLINE_DECL_REFCOUNTING(AnonymousCounterStyle, override)

private:
  ~AnonymousCounterStyle() {}

  bool mSingleString;
  uint8_t mSystem;
  nsTArray<nsString> mSymbols;
};

class CounterStyleManager final
{
private:
  ~CounterStyleManager();
public:
  explicit CounterStyleManager(nsPresContext* aPresContext);

  static void InitializeBuiltinCounterStyles();

  void Disconnect();

  bool IsInitial() const
  {
    
    return mCacheTable.Count() == 2;
  }

  CounterStyle* BuildCounterStyle(const nsSubstring& aName);

  static CounterStyle* GetBuiltinStyle(int32_t aStyle);
  static CounterStyle* GetNoneStyle()
  {
    return GetBuiltinStyle(NS_STYLE_LIST_STYLE_NONE);
  }
  static CounterStyle* GetDecimalStyle()
  {
    return GetBuiltinStyle(NS_STYLE_LIST_STYLE_DECIMAL);
  }

  
  
  
  
  bool NotifyRuleChanged();

  nsPresContext* PresContext() const { return mPresContext; }

  NS_INLINE_DECL_REFCOUNTING(CounterStyleManager)

private:
  nsPresContext* mPresContext;
  nsRefPtrHashtable<nsStringHashKey, CounterStyle> mCacheTable;
};

} 

#endif 
