




#ifndef mozilla_dom_responsiveimageselector_h__
#define mozilla_dom_responsiveimageselector_h__

#include "nsISupports.h"
#include "nsIContent.h"
#include "nsString.h"

class nsMediaQuery;
class nsCSSValue;

namespace mozilla {
namespace dom {

class ResponsiveImageCandidate;

class ResponsiveImageSelector : public nsISupports
{
  friend class ResponsiveImageCandidate;
public:
  NS_DECL_ISUPPORTS
  ResponsiveImageSelector(nsIContent *aContent);

  
  
  bool SetCandidatesFromSourceSet(const nsAString & aSrcSet);

  
  
  bool SetSizesFromDescriptor(const nsAString & aSizesDescriptor);

  
  nsresult SetDefaultSource(const nsAString & aSpec);
  void SetDefaultSource(nsIURI *aURL);

  uint32_t NumCandidates(bool aIncludeDefault = true);

  nsIContent *Content() { return mContent; }

  
  already_AddRefed<nsIURI> GetSelectedImageURL();
  double GetSelectedImageDensity();

protected:
  virtual ~ResponsiveImageSelector();

private:
  
  
  void AppendCandidateIfUnique(const ResponsiveImageCandidate &aCandidate);

  
  
  void AppendDefaultCandidate(nsIURI *aURL);

  
  int GetBestCandidateIndex();

  
  
  
  
  
  bool ComputeFinalWidthForCurrentViewport(int32_t *aWidth);

  nsCOMPtr<nsIContent> mContent;
  
  
  nsTArray<ResponsiveImageCandidate> mCandidates;
  int mBestCandidateIndex;

  nsTArray< nsAutoPtr<nsMediaQuery> > mSizeQueries;
  nsTArray<nsCSSValue> mSizeValues;
};

class ResponsiveImageCandidate {
public:
  ResponsiveImageCandidate();
  ResponsiveImageCandidate(nsIURI *aURL, double aDensity);

  void SetURL(nsIURI *aURL);
  
  
  
  void SetParameterDefault();

  
  void SetParameterAsDensity(double aDensity);
  void SetParameterAsComputedWidth(int32_t aWidth);

  
  
  bool SetParamaterFromDescriptor(const nsAString & aDescriptor);

  
  bool HasSameParameter(const ResponsiveImageCandidate & aOther) const;

  already_AddRefed<nsIURI> URL() const;

  
  double Density(ResponsiveImageSelector *aSelector) const;
  
  
  double Density(int32_t aMatchingWidth) const;

  
  bool IsComputedFromWidth() const;

  enum eCandidateType {
    eCandidateType_Invalid,
    eCandidateType_Density,
    
    
    eCandidateType_Default,
    eCandidateType_ComputedFromWidth
  };

  eCandidateType Type() const { return mType; }

private:

  nsCOMPtr<nsIURI> mURL;
  eCandidateType mType;
  union {
    double mDensity;
    int32_t mWidth;
  } mValue;
};

} 
} 

#endif 
