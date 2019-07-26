




#include "mozilla/dom/ResponsiveImageSelector.h"
#include "nsIURI.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"
#include "nsPresContext.h"
#include "nsNetUtil.h"

using namespace mozilla;
using namespace mozilla::dom;

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS0(ResponsiveImageSelector);

ResponsiveImageSelector::ResponsiveImageSelector(nsIContent *aContent)
  : mContent(aContent),
    mBestCandidateIndex(-1)
{
}

ResponsiveImageSelector::~ResponsiveImageSelector()
{}


bool
ResponsiveImageSelector::SetCandidatesFromSourceSet(const nsAString & aSrcSet)
{
  nsIDocument* doc = mContent ? mContent->OwnerDoc() : nullptr;
  nsCOMPtr<nsIURI> docBaseURI = mContent ? mContent->GetBaseURI() : nullptr;

  if (!mContent || !doc || !docBaseURI) {
    MOZ_ASSERT(false,
               "Should not be parsing SourceSet without a content and document");
    return false;
  }

  
  uint32_t prevNumCandidates = mCandidates.Length();
  nsCOMPtr<nsIURI> defaultURL;
  if (prevNumCandidates && (mCandidates[prevNumCandidates - 1].Type() ==
                            ResponsiveImageCandidate::eCandidateType_Default)) {
    defaultURL = mCandidates[prevNumCandidates - 1].URL();
  }

  mCandidates.Clear();

  nsAString::const_iterator iter, end;
  aSrcSet.BeginReading(iter);
  aSrcSet.EndReading(end);

  
  while (iter != end) {
    nsAString::const_iterator url, desc;

    
    for (; iter != end && nsContentUtils::IsHTMLWhitespace(*iter); ++iter);

    if (iter == end) {
      break;
    }

    url = iter;

    
    for (;iter != end && !nsContentUtils::IsHTMLWhitespace(*iter); ++iter);

    desc = iter;

    
    for (; iter != end && *iter != char16_t(','); ++iter);
    const nsDependentSubstring &descriptor = Substring(desc, iter);

    nsresult rv;
    nsCOMPtr<nsIURI> candidateURL;
    rv = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(candidateURL),
                                                   Substring(url, desc),
                                                   doc,
                                                   docBaseURI);
    if (NS_SUCCEEDED(rv) && candidateURL) {
      NS_TryToSetImmutable(candidateURL);
      ResponsiveImageCandidate candidate;
      if (candidate.SetParamaterFromDescriptor(descriptor)) {
        candidate.SetURL(candidateURL);
        AppendCandidateIfUnique(candidate);
      }
    }

    
    if (iter != end) {
      ++iter;
    }
  }

  bool parsedCandidates = mCandidates.Length() > 0;

  
  if (defaultURL) {
    AppendDefaultCandidate(defaultURL);
  }

  return parsedCandidates;
}

nsresult
ResponsiveImageSelector::SetDefaultSource(const nsAString & aSpec)
{
  nsIDocument* doc = mContent ? mContent->OwnerDoc() : nullptr;
  nsCOMPtr<nsIURI> docBaseURI = mContent ? mContent->GetBaseURI() : nullptr;

  if (!mContent || !doc || !docBaseURI) {
    MOZ_ASSERT(false,
               "Should not be calling this without a content and document");
    return NS_ERROR_UNEXPECTED;
  }

  if (aSpec.IsEmpty()) {
    SetDefaultSource(nullptr);
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsIURI> candidateURL;
  rv = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(candidateURL),
                                                 aSpec, doc, docBaseURI);
  NS_ENSURE_SUCCESS(rv, rv);

  SetDefaultSource(candidateURL);
  return NS_OK;
}

void
ResponsiveImageSelector::SetDefaultSource(nsIURI *aURL)
{
  
  int32_t candidates = mCandidates.Length();
  if (candidates && (mCandidates[candidates - 1].Type() ==
                     ResponsiveImageCandidate::eCandidateType_Default)) {
    mCandidates.RemoveElementAt(candidates - 1);
    if (mBestCandidateIndex == candidates - 1) {
      mBestCandidateIndex = -1;
    }
  }

  
  if (aURL) {
    AppendDefaultCandidate(aURL);
  }
}

void
ResponsiveImageSelector::AppendCandidateIfUnique(const ResponsiveImageCandidate & aCandidate)
{
  
  int numCandidates = mCandidates.Length();
  for (int i = 0; i < numCandidates; i++) {
    if (mCandidates[i].HasSameParameter(aCandidate)) {
      return;
    }
  }

  mBestCandidateIndex = -1;
  mCandidates.AppendElement(aCandidate);
}

void
ResponsiveImageSelector::AppendDefaultCandidate(nsIURI *aURL)
{
  NS_ENSURE_TRUE(aURL, );

  ResponsiveImageCandidate defaultCandidate;
  defaultCandidate.SetParameterDefault();
  defaultCandidate.SetURL(aURL);
  
  
  mBestCandidateIndex = -1;
  mCandidates.AppendElement(defaultCandidate);
}

already_AddRefed<nsIURI>
ResponsiveImageSelector::GetSelectedImageURL()
{
  int bestIndex = GetBestCandidateIndex();
  if (bestIndex < 0) {
    return nullptr;
  }

  nsCOMPtr<nsIURI> bestURL = mCandidates[bestIndex].URL();
  MOZ_ASSERT(bestURL, "Shouldn't have candidates with no URL in the array");
  return bestURL.forget();
}

double
ResponsiveImageSelector::GetSelectedImageDensity()
{
  int bestIndex = GetBestCandidateIndex();
  if (bestIndex < 0) {
    return 1.0;
  }

  return mCandidates[bestIndex].Density();
}

int
ResponsiveImageSelector::GetBestCandidateIndex()
{
  if (mBestCandidateIndex != -1) {
    return mBestCandidateIndex;
  }

  int numCandidates = mCandidates.Length();
  if (!numCandidates) {
    return -1;
  }

  nsIDocument* doc = mContent ? mContent->OwnerDoc() : nullptr;
  nsIPresShell *shell = doc ? doc->GetShell() : nullptr;
  nsPresContext *pctx = shell ? shell->GetPresContext() : nullptr;

  if (!pctx) {
    MOZ_ASSERT(false, "Unable to find document prescontext");
    return -1;
  }

  double displayDensity = pctx->CSSPixelsToDevPixels(1);

  
  
  

  int bestIndex = 0; 
  double bestDensity = mCandidates[bestIndex].Density();
  for (int i = 1; i < numCandidates; i++) {
    double candidateDensity = mCandidates[i].Density();
    
    
    
    if ((bestDensity < displayDensity && candidateDensity > bestDensity) ||
        (candidateDensity > displayDensity && candidateDensity < bestDensity)) {
      bestIndex = i;
      bestDensity = candidateDensity;
    }
  }

  mBestCandidateIndex = bestIndex;
  return bestIndex;
}

ResponsiveImageCandidate::ResponsiveImageCandidate()
{
  mType = eCandidateType_Invalid;
  mValue.mDensity = 1.0;
}

ResponsiveImageCandidate::ResponsiveImageCandidate(nsIURI *aURL,
                                                   double aDensity)
  : mURL(aURL)
{
  mType = eCandidateType_Density;
  mValue.mDensity = aDensity;
}


void
ResponsiveImageCandidate::SetURL(nsIURI *aURL)
{
  mURL = aURL;
}

void
ResponsiveImageCandidate::SetParameterDefault()
{
  MOZ_ASSERT(mType == eCandidateType_Invalid, "double setting candidate type");

  mType = eCandidateType_Default;
  
  
  mValue.mDensity = 1.0;
}

void
ResponsiveImageCandidate::SetParameterAsDensity(double aDensity)
{
  MOZ_ASSERT(mType == eCandidateType_Invalid, "double setting candidate type");

  mType = eCandidateType_Density;
  mValue.mDensity = aDensity;
}

bool
ResponsiveImageCandidate::SetParamaterFromDescriptor(const nsAString & aDescriptor)
{
  
  double density = -1.0;

  nsAString::const_iterator iter, end;
  aDescriptor.BeginReading(iter);
  aDescriptor.EndReading(end);

  
  
  
  
  while (iter != end) {
    
    for (; iter != end && nsContentUtils::IsHTMLWhitespace(*iter); ++iter);
    if (iter == end) {
      break;
    }

    
    nsAString::const_iterator start = iter;
    for (; iter != end && !nsContentUtils::IsHTMLWhitespace(*iter); ++iter);

    if (start == iter) {
      
      break;
    }

    
    
    --iter;
    nsAString::const_iterator type(iter);
    ++iter;

    const nsDependentSubstring& valStr = Substring(start, type);
    if (*type == char16_t('x')) {
      nsresult rv;
      double possibleDensity = PromiseFlatString(valStr).ToDouble(&rv);
      if (density == -1.0 && NS_SUCCEEDED(rv) && possibleDensity > 0.0) {
        density = possibleDensity;
      } else {
        return false;
      }
    }
  }

  
  if (density == -1.0) {
    density = 1.0;
  }

  SetParameterAsDensity(density);
  return true;
}

bool
ResponsiveImageCandidate::HasSameParameter(const ResponsiveImageCandidate & aOther) const
{
  if (aOther.mType != mType) {
    return false;
  }

  if (mType == eCandidateType_Default) {
    return true;
  }

  if (mType == eCandidateType_Density) {
    return aOther.mValue.mDensity == mValue.mDensity;
  }

  if (mType == eCandidateType_Invalid) {
    MOZ_ASSERT(false, "Comparing invalid candidates?");
    return true;
  }

  MOZ_ASSERT(false, "Somebody forgot to check for all uses of this enum");
  return false;
}

double
ResponsiveImageCandidate::Density() const
{
  
  if (mType == eCandidateType_Invalid) {
    MOZ_ASSERT(false, "Getting density for uninitialized candidate");
    return 1.0;
  }

  if (mType == eCandidateType_Default) {
    return 1.0;
  }

  if (mType == eCandidateType_Density) {
    return mValue.mDensity;
  }

  MOZ_ASSERT(false, "Bad candidate type in Density()");
  return 1.0;
}

already_AddRefed<nsIURI>
ResponsiveImageCandidate::URL() const
{
  MOZ_ASSERT(mType != eCandidateType_Invalid,
             "Getting URL of incomplete candidate");
  nsCOMPtr<nsIURI> url = mURL;
  return url.forget();
}

} 
} 
