










#ifndef nsIMediaList_h_
#define nsIMediaList_h_

#include "nsIDOMMediaList.h"
#include "nsTArray.h"
#include "nsIAtom.h"
#include "nsCSSValue.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"

class nsPresContext;
class nsAString;
struct nsMediaFeature;

namespace mozilla {
class CSSStyleSheet;
} 

struct nsMediaExpression {
  enum Range { eMin, eMax, eEqual };

  const nsMediaFeature *mFeature;
  Range mRange;
  nsCSSValue mValue;

  
  bool Matches(nsPresContext* aPresContext,
                 const nsCSSValue& aActualValue) const;
};




















class nsMediaQueryResultCacheKey {
public:
  explicit nsMediaQueryResultCacheKey(nsIAtom* aMedium)
    : mMedium(aMedium)
  {}

  




  void AddExpression(const nsMediaExpression* aExpression,
                     bool aExpressionMatches);
  bool Matches(nsPresContext* aPresContext) const;
private:
  struct ExpressionEntry {
    
    
    
    nsMediaExpression mExpression;
    bool mExpressionMatches;
  };
  struct FeatureEntry {
    const nsMediaFeature *mFeature;
    InfallibleTArray<ExpressionEntry> mExpressions;
  };
  nsCOMPtr<nsIAtom> mMedium;
  nsTArray<FeatureEntry> mFeatureCache;
};

class nsMediaQuery {
public:
  nsMediaQuery()
    : mNegated(false)
    , mHasOnly(false)
    , mTypeOmitted(false)
    , mHadUnknownExpression(false)
  {
  }

private:
  
  nsMediaQuery(const nsMediaQuery& aOther)
    : mNegated(aOther.mNegated)
    , mHasOnly(aOther.mHasOnly)
    , mTypeOmitted(aOther.mTypeOmitted)
    , mHadUnknownExpression(aOther.mHadUnknownExpression)
    , mMediaType(aOther.mMediaType)
    , mExpressions(aOther.mExpressions)
  {
    MOZ_ASSERT(mExpressions.Length() == aOther.mExpressions.Length());
  }

public:

  void SetNegated()                     { mNegated = true; }
  void SetHasOnly()                     { mHasOnly = true; }
  void SetTypeOmitted()                 { mTypeOmitted = true; }
  void SetHadUnknownExpression()        { mHadUnknownExpression = true; }
  void SetType(nsIAtom* aMediaType)     { 
                                          NS_ASSERTION(aMediaType,
                                                       "expected non-null");
                                          mMediaType = aMediaType;
                                        }

  
  
  
  
  nsMediaExpression* NewExpression()    { return mExpressions.AppendElement(); }

  void AppendToString(nsAString& aString) const;

  nsMediaQuery* Clone() const;

  
  
  bool Matches(nsPresContext* aPresContext,
                 nsMediaQueryResultCacheKey* aKey) const;

private:
  bool mNegated;
  bool mHasOnly; 
  bool mTypeOmitted; 
  bool mHadUnknownExpression;
  nsCOMPtr<nsIAtom> mMediaType;
  nsTArray<nsMediaExpression> mExpressions;
};

class nsMediaList MOZ_FINAL : public nsIDOMMediaList
                            , public nsWrapperCache
{
public:
  typedef mozilla::ErrorResult ErrorResult;

  nsMediaList();

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;
  nsISupports* GetParentObject() const
  {
    return nullptr;
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsMediaList)

  NS_DECL_NSIDOMMEDIALIST

  void GetText(nsAString& aMediaText);
  void SetText(const nsAString& aMediaText);

  
  
  bool Matches(nsPresContext* aPresContext,
                 nsMediaQueryResultCacheKey* aKey);

  void SetStyleSheet(mozilla::CSSStyleSheet* aSheet);
  void AppendQuery(nsAutoPtr<nsMediaQuery>& aQuery) {
    
    mArray.AppendElement(aQuery.forget());
  }

  already_AddRefed<nsMediaList> Clone();

  nsMediaQuery* MediumAt(int32_t aIndex) { return mArray[aIndex]; }
  void Clear() { mArray.Clear(); }

  
  
  uint32_t Length() { return mArray.Length(); }
  void IndexedGetter(uint32_t aIndex, bool& aFound, nsAString& aReturn);
  
  void DeleteMedium(const nsAString& aMedium, ErrorResult& aRv)
  {
    aRv = DeleteMedium(aMedium);
  }
  void AppendMedium(const nsAString& aMedium, ErrorResult& aRv)
  {
    aRv = AppendMedium(aMedium);
  }

protected:
  ~nsMediaList();

  nsresult Delete(const nsAString & aOldMedium);
  nsresult Append(const nsAString & aOldMedium);

  InfallibleTArray<nsAutoPtr<nsMediaQuery> > mArray;
  
  
  
  mozilla::CSSStyleSheet* mStyleSheet;
};
#endif 
