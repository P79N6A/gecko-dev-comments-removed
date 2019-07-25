










#ifndef nsIMediaList_h_
#define nsIMediaList_h_

#include "nsIDOMMediaList.h"
#include "nsTArray.h"
#include "nsIAtom.h"
#include "nsCSSValue.h"

class nsPresContext;
class nsCSSStyleSheet;
class nsAString;
struct nsMediaFeature;

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
  nsMediaQueryResultCacheKey(nsIAtom* aMedium)
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

class nsMediaList : public nsIDOMMediaList {
public:
  nsMediaList();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMMEDIALIST

  nsresult GetText(nsAString& aMediaText);
  nsresult SetText(const nsAString& aMediaText);

  
  
  bool Matches(nsPresContext* aPresContext,
                 nsMediaQueryResultCacheKey* aKey);

  nsresult SetStyleSheet(nsCSSStyleSheet* aSheet);
  void AppendQuery(nsAutoPtr<nsMediaQuery>& aQuery) {
    
    mArray.AppendElement(aQuery.forget());
  }

  nsresult Clone(nsMediaList** aResult);

  PRInt32 Count() { return mArray.Length(); }
  nsMediaQuery* MediumAt(PRInt32 aIndex) { return mArray[aIndex]; }
  void Clear() { mArray.Clear(); }

protected:
  ~nsMediaList();

  nsresult Delete(const nsAString & aOldMedium);
  nsresult Append(const nsAString & aOldMedium);

  InfallibleTArray<nsAutoPtr<nsMediaQuery> > mArray;
  
  
  
  nsCSSStyleSheet*         mStyleSheet;
};
#endif 
