




#ifndef mozilla_dom_responsiveimageselector_h__
#define mozilla_dom_responsiveimageselector_h__

#include "nsISupports.h"
#include "nsIContent.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

class ResponsiveImageCandidate;

class ResponsiveImageSelector : public nsISupports
{
public:
  NS_DECL_ISUPPORTS
  ResponsiveImageSelector(nsIContent *aContent);
  virtual ~ResponsiveImageSelector();

  
  
  bool SetCandidatesFromSourceSet(const nsAString & aSrcSet);

  
  nsresult SetDefaultSource(const nsAString & aSpec);
  void SetDefaultSource(nsIURI *aURL);

  
  already_AddRefed<nsIURI> GetSelectedImageURL();
  double GetSelectedImageDensity();

private:
  
  
  void AppendCandidateIfUnique(const ResponsiveImageCandidate &aCandidate);

  
  
  void AppendDefaultCandidate(nsIURI *aURL);

  
  int GetBestCandidateIndex();

  nsCOMPtr<nsIContent> mContent;
  
  
  nsTArray<ResponsiveImageCandidate> mCandidates;
  int mBestCandidateIndex;
};

class ResponsiveImageCandidate {
public:
  ResponsiveImageCandidate();
  ResponsiveImageCandidate(nsIURI *aURL, double aDensity);

  void SetURL(nsIURI *aURL);
  
  
  
  void SetParameterDefault();

  
  void SetParameterAsDensity(double aDensity);

  
  
  bool SetParamaterFromDescriptor(const nsAString & aDescriptor);

  
  bool HasSameParameter(const ResponsiveImageCandidate & aOther) const;

  already_AddRefed<nsIURI> URL() const;
  double Density() const;

  enum eCandidateType {
    eCandidateType_Invalid,
    eCandidateType_Density,
    
    
    eCandidateType_Default,
  };

  eCandidateType Type() const { return mType; }

private:

  nsCOMPtr<nsIURI> mURL;
  eCandidateType mType;
  union {
    double mDensity;
  } mValue;
};

} 
} 

#endif 
