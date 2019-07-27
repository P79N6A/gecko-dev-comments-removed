




#ifndef mozilla_dom_responsiveimageselector_h__
#define mozilla_dom_responsiveimageselector_h__

#include "nsISupports.h"
#include "nsIContent.h"
#include "nsString.h"
#include "nsCycleCollectionParticipant.h"

class nsMediaQuery;
class nsCSSValue;

namespace mozilla {
namespace dom {

class ResponsiveImageCandidate;

class ResponsiveImageSelector
{
  friend class ResponsiveImageCandidate;
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(ResponsiveImageSelector)
  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(ResponsiveImageSelector)

  explicit ResponsiveImageSelector(nsIContent* aContent);
  explicit ResponsiveImageSelector(nsIDocument* aDocument);

  
  
  
  
  
  
  
  
  
  


  
  
  bool SetCandidatesFromSourceSet(const nsAString & aSrcSet);

  
  
  bool SetSizesFromDescriptor(const nsAString & aSizesDescriptor);

  
  void SetDefaultSource(const nsAString& aURLString);

  uint32_t NumCandidates(bool aIncludeDefault = true);

  
  
  nsIContent *Content();

  
  
  nsIDocument *Document();

  
  
  already_AddRefed<nsIURI> GetSelectedImageURL();
  
  bool GetSelectedImageURLSpec(nsAString& aResult);
  double GetSelectedImageDensity();

  
  
  
  
  
  
  bool SelectImage(bool aReselect = false);

protected:
  virtual ~ResponsiveImageSelector();

private:
  
  
  void AppendCandidateIfUnique(const ResponsiveImageCandidate &aCandidate);

  
  
  void AppendDefaultCandidate(const nsAString& aURLString);

  
  int GetSelectedCandidateIndex();

  
  
  void ClearSelectedCandidate();

  
  
  
  
  
  bool ComputeFinalWidthForCurrentViewport(int32_t *aWidth);

  nsCOMPtr<nsINode> mOwnerNode;
  
  
  nsTArray<ResponsiveImageCandidate> mCandidates;
  int mSelectedCandidateIndex;
  
  
  nsCOMPtr<nsIURI> mSelectedCandidateURL;

  nsTArray< nsAutoPtr<nsMediaQuery> > mSizeQueries;
  nsTArray<nsCSSValue> mSizeValues;
};

class ResponsiveImageCandidate {
public:
  ResponsiveImageCandidate();
  ResponsiveImageCandidate(const nsAString& aURLString, double aDensity);

  void SetURLSpec(const nsAString& aURLString);
  
  
  
  void SetParameterDefault();

  
  void SetParameterAsDensity(double aDensity);
  void SetParameterAsComputedWidth(int32_t aWidth);

  void SetParameterInvalid();

  
  
  
  
  bool ConsumeDescriptors(nsAString::const_iterator& aIter,
                          const nsAString::const_iterator& aIterEnd);

  
  bool HasSameParameter(const ResponsiveImageCandidate & aOther) const;

  const nsAString& URLString() const;

  
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

  nsString mURLString;
  eCandidateType mType;
  union {
    double mDensity;
    int32_t mWidth;
  } mValue;
};

} 
} 

#endif 
