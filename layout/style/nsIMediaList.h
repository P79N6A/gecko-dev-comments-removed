











































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

  
  PRBool Matches(nsPresContext* aPresContext,
                 const nsCSSValue& aActualValue) const;
};




















class nsMediaQueryResultCacheKey {
public:
  nsMediaQueryResultCacheKey(nsIAtom* aMedium)
    : mMedium(aMedium)
  {}

  




  void AddExpression(const nsMediaExpression* aExpression,
                     PRBool aExpressionMatches);
  PRBool Matches(nsPresContext* aPresContext) const;
private:
  struct ExpressionEntry {
    
    
    
    nsMediaExpression mExpression;
    PRBool mExpressionMatches;
  };
  struct FeatureEntry {
    const nsMediaFeature *mFeature;
    nsTArray<ExpressionEntry> mExpressions;
  };
  nsCOMPtr<nsIAtom> mMedium;
  nsTArray<FeatureEntry> mFeatureCache;
};

class nsMediaQuery {
public:
  nsMediaQuery()
    : mNegated(PR_FALSE)
    , mHasOnly(PR_FALSE)
    , mTypeOmitted(PR_FALSE)
    , mHadUnknownExpression(PR_FALSE)
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

  void SetNegated()                     { mNegated = PR_TRUE; }
  void SetHasOnly()                     { mHasOnly = PR_TRUE; }
  void SetTypeOmitted()                 { mTypeOmitted = PR_TRUE; }
  void SetHadUnknownExpression()        { mHadUnknownExpression = PR_TRUE; }
  void SetType(nsIAtom* aMediaType)     { 
                                          NS_ASSERTION(aMediaType,
                                                       "expected non-null");
                                          mMediaType = aMediaType;
                                        }

  
  
  
  
  nsMediaExpression* NewExpression()    { return mExpressions.AppendElement(); }

  void AppendToString(nsAString& aString) const;

  nsMediaQuery* Clone() const;

  
  
  PRBool Matches(nsPresContext* aPresContext,
                 nsMediaQueryResultCacheKey* aKey) const;

private:
  PRPackedBool mNegated;
  PRPackedBool mHasOnly; 
  PRPackedBool mTypeOmitted; 
  PRPackedBool mHadUnknownExpression;
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

  
  
  PRBool Matches(nsPresContext* aPresContext,
                 nsMediaQueryResultCacheKey* aKey);

  nsresult SetStyleSheet(nsCSSStyleSheet* aSheet);
  nsresult AppendQuery(nsAutoPtr<nsMediaQuery>& aQuery) {
    
    if (!mArray.AppendElement(aQuery.get())) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    aQuery.forget();
    return NS_OK;
  }

  nsresult Clone(nsMediaList** aResult);

  PRInt32 Count() { return mArray.Length(); }
  nsMediaQuery* MediumAt(PRInt32 aIndex) { return mArray[aIndex]; }
  void Clear() { mArray.Clear(); mIsEmpty = PR_TRUE; }
  
  
  
  void SetNonEmpty() { mIsEmpty = PR_FALSE; }

protected:
  ~nsMediaList();

  nsresult Delete(const nsAString & aOldMedium);
  nsresult Append(const nsAString & aOldMedium);

  nsTArray<nsAutoPtr<nsMediaQuery> > mArray;
  PRBool mIsEmpty;
  
  
  
  nsCSSStyleSheet*         mStyleSheet;
};
#endif 
