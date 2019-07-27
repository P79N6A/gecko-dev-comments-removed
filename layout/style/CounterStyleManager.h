




#ifndef mozilla_CounterStyleManager_h_
#define mozilla_CounterStyleManager_h_

#include "nsStringFwd.h"
#include "nsRefPtrHashtable.h"
#include "nsHashKeys.h"

#include "nsStyleConsts.h"

#include "mozilla/NullPtr.h"
#include "mozilla/Attributes.h"

#include "nsCSSValue.h"

class nsPresContext;
class nsCSSCounterStyleRule;

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
  CounterStyle(const CounterStyle& aOther) MOZ_DELETE;
  void operator=(const CounterStyle& other) MOZ_DELETE;

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

class AnonymousCounterStyle MOZ_FINAL : public CounterStyle
{
public:
  explicit AnonymousCounterStyle(const nsCSSValue::Array* aValue);

  virtual void GetPrefix(nsAString& aResult) MOZ_OVERRIDE;
  virtual void GetSuffix(nsAString& aResult) MOZ_OVERRIDE;
  virtual bool IsBullet() MOZ_OVERRIDE;

  virtual void GetNegative(NegativeType& aResult) MOZ_OVERRIDE;
  virtual bool IsOrdinalInRange(CounterValue aOrdinal) MOZ_OVERRIDE;
  virtual bool IsOrdinalInAutoRange(CounterValue aOrdinal) MOZ_OVERRIDE;
  virtual void GetPad(PadType& aResult) MOZ_OVERRIDE;
  virtual CounterStyle* GetFallback() MOZ_OVERRIDE;
  virtual uint8_t GetSpeakAs() MOZ_OVERRIDE;
  virtual bool UseNegativeSign() MOZ_OVERRIDE;

  virtual bool GetInitialCounterText(CounterValue aOrdinal,
                                     WritingMode aWritingMode,
                                     nsSubstring& aResult,
                                     bool& aIsRTL) MOZ_OVERRIDE;

  virtual AnonymousCounterStyle* AsAnonymous() MOZ_OVERRIDE { return this; }

  uint8_t GetSystem() const { return mSystem; }
  const nsTArray<nsString>& GetSymbols() const { return mSymbols; }

  NS_INLINE_DECL_REFCOUNTING(AnonymousCounterStyle)

private:
  ~AnonymousCounterStyle() {}

  uint8_t mSystem;
  nsTArray<nsString> mSymbols;
};

class CounterStyleManager MOZ_FINAL
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
  CounterStyle* BuildCounterStyle(const nsCSSValue::Array* aParams);

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
