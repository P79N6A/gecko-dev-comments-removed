




#include "mozilla/dom/ResponsiveImageSelector.h"
#include "nsIURI.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"
#include "nsPresContext.h"
#include "nsNetUtil.h"

#include "nsCSSParser.h"
#include "nsCSSProps.h"
#include "nsIMediaList.h"
#include "nsRuleNode.h"
#include "nsRuleData.h"



#include "HTMLPictureElement.h"

using namespace mozilla;
using namespace mozilla::dom;

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION(ResponsiveImageSelector, mOwnerNode)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(ResponsiveImageSelector, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(ResponsiveImageSelector, Release)

static bool
ParseInteger(const nsAString& aString, int32_t& aInt)
{
  nsContentUtils::ParseHTMLIntegerResultFlags parseResult;
  aInt = nsContentUtils::ParseHTMLInteger(aString, &parseResult);
  return !(parseResult &
           ( nsContentUtils::eParseHTMLInteger_Error |
             nsContentUtils::eParseHTMLInteger_DidNotConsumeAllInput |
             nsContentUtils::eParseHTMLInteger_IsPercent |
             nsContentUtils::eParseHTMLInteger_NonStandard ));
}

ResponsiveImageSelector::ResponsiveImageSelector(nsIContent *aContent)
  : mOwnerNode(aContent),
    mSelectedCandidateIndex(-1)
{
}

ResponsiveImageSelector::ResponsiveImageSelector(nsIDocument *aDocument)
  : mOwnerNode(aDocument),
    mSelectedCandidateIndex(-1)
{
}

ResponsiveImageSelector::~ResponsiveImageSelector()
{}


bool
ResponsiveImageSelector::SetCandidatesFromSourceSet(const nsAString & aSrcSet)
{
  ClearSelectedCandidate();

  nsCOMPtr<nsIURI> docBaseURI = mOwnerNode ? mOwnerNode->GetBaseURI() : nullptr;

  if (!docBaseURI) {
    MOZ_ASSERT(false,
               "Should not be parsing SourceSet without a document");
    return false;
  }

  
  uint32_t prevNumCandidates = mCandidates.Length();
  nsString defaultURLString;
  if (prevNumCandidates && (mCandidates[prevNumCandidates - 1].Type() ==
                            ResponsiveImageCandidate::eCandidateType_Default)) {
    defaultURLString = mCandidates[prevNumCandidates - 1].URLString();
  }

  mCandidates.Clear();

  nsAString::const_iterator iter, end;
  aSrcSet.BeginReading(iter);
  aSrcSet.EndReading(end);

  
  while (iter != end) {
    nsAString::const_iterator url, urlEnd, descriptor;

    
    
    for (; iter != end && (nsContentUtils::IsHTMLWhitespace(*iter) ||
                           *iter == char16_t(',')); ++iter);

    if (iter == end) {
      break;
    }

    url = iter;

    
    for (;iter != end && !nsContentUtils::IsHTMLWhitespace(*iter); ++iter);

    
    
    while (iter != url) {
      if (*(--iter) != char16_t(',')) {
        iter++;
        break;
      }
    }

    const nsDependentSubstring &urlStr = Substring(url, iter);

    MOZ_ASSERT(url != iter, "Shouldn't have empty URL at this point");

    ResponsiveImageCandidate candidate;
    if (candidate.ConsumeDescriptors(iter, end)) {
      candidate.SetURLSpec(urlStr);
      AppendCandidateIfUnique(candidate);
    }
  }

  bool parsedCandidates = mCandidates.Length() > 0;

  
  if (!defaultURLString.IsEmpty()) {
    AppendDefaultCandidate(defaultURLString);
  }

  return parsedCandidates;
}

uint32_t
ResponsiveImageSelector::NumCandidates(bool aIncludeDefault)
{
  uint32_t candidates = mCandidates.Length();

  
  if (!aIncludeDefault && candidates &&
      (mCandidates[candidates - 1].Type() ==
       ResponsiveImageCandidate::eCandidateType_Default)) {
    candidates--;
  }

  return candidates;
}

nsIContent*
ResponsiveImageSelector::Content()
{
  return mOwnerNode->IsContent() ? mOwnerNode->AsContent() : nullptr;
}

nsIDocument*
ResponsiveImageSelector::Document()
{
  return mOwnerNode->OwnerDoc();
}

void
ResponsiveImageSelector::SetDefaultSource(const nsAString& aURLString)
{
  ClearSelectedCandidate();

  
  int32_t candidates = mCandidates.Length();
  if (candidates && (mCandidates[candidates - 1].Type() ==
                     ResponsiveImageCandidate::eCandidateType_Default)) {
    mCandidates.RemoveElementAt(candidates - 1);
  }

  
  if (!aURLString.IsEmpty()) {
    AppendDefaultCandidate(aURLString);
  }
}

void
ResponsiveImageSelector::ClearSelectedCandidate()
{
  mSelectedCandidateIndex = -1;
  mSelectedCandidateURL = nullptr;
}

bool
ResponsiveImageSelector::SetSizesFromDescriptor(const nsAString & aSizes)
{
  ClearSelectedCandidate();
  mSizeQueries.Clear();
  mSizeValues.Clear();

  nsCSSParser cssParser;

  if (!cssParser.ParseSourceSizeList(aSizes, nullptr, 0,
                                     mSizeQueries, mSizeValues, true)) {
    return false;
  }

  return mSizeQueries.Length() > 0;
}

void
ResponsiveImageSelector::AppendCandidateIfUnique(const ResponsiveImageCandidate & aCandidate)
{
  int numCandidates = mCandidates.Length();

  
  
  
  if (numCandidates && mCandidates[0].Type() != aCandidate.Type()) {
    return;
  }

  
  for (int i = 0; i < numCandidates; i++) {
    if (mCandidates[i].HasSameParameter(aCandidate)) {
      return;
    }
  }

  mCandidates.AppendElement(aCandidate);
}

void
ResponsiveImageSelector::AppendDefaultCandidate(const nsAString& aURLString)
{
  NS_ENSURE_TRUE(!aURLString.IsEmpty(), );

  ResponsiveImageCandidate defaultCandidate;
  defaultCandidate.SetParameterDefault();
  defaultCandidate.SetURLSpec(aURLString);
  
  
  mCandidates.AppendElement(defaultCandidate);
}

already_AddRefed<nsIURI>
ResponsiveImageSelector::GetSelectedImageURL()
{
  SelectImage();

  nsCOMPtr<nsIURI> url = mSelectedCandidateURL;
  return url.forget();
}

bool
ResponsiveImageSelector::GetSelectedImageURLSpec(nsAString& aResult)
{
  SelectImage();

  if (mSelectedCandidateIndex == -1) {
    return false;
  }

  aResult.Assign(mCandidates[mSelectedCandidateIndex].URLString());
  return true;
}

double
ResponsiveImageSelector::GetSelectedImageDensity()
{
  int bestIndex = GetSelectedCandidateIndex();
  if (bestIndex < 0) {
    return 1.0;
  }

  return mCandidates[bestIndex].Density(this);
}

bool
ResponsiveImageSelector::SelectImage(bool aReselect)
{
  if (!aReselect && mSelectedCandidateIndex != -1) {
    
    return false;
  }

  int oldBest = mSelectedCandidateIndex;
  ClearSelectedCandidate();

  int numCandidates = mCandidates.Length();
  if (!numCandidates) {
    return oldBest != -1;
  }

  nsIDocument* doc = Document();
  nsIPresShell *shell = doc ? doc->GetShell() : nullptr;
  nsPresContext *pctx = shell ? shell->GetPresContext() : nullptr;
  nsCOMPtr<nsIURI> baseURI = mOwnerNode ? mOwnerNode->GetBaseURI() : nullptr;

  if (!pctx || !doc || !baseURI) {
    MOZ_ASSERT(false, "Unable to find document prescontext and base URI");
    return oldBest != -1;
  }

  double displayDensity = pctx->CSSPixelsToDevPixels(1.0f);

  
  
  

  
  
  
  
  int32_t computedWidth = -1;
  if (numCandidates && mCandidates[0].IsComputedFromWidth()) {
    DebugOnly<bool> computeResult = \
      ComputeFinalWidthForCurrentViewport(&computedWidth);
    MOZ_ASSERT(computeResult,
               "Computed candidates not allowed without sizes data");

    
    
    
    if (numCandidates > 1 && mCandidates[numCandidates - 1].Type() ==
        ResponsiveImageCandidate::eCandidateType_Default) {
      numCandidates--;
    }
  }

  int bestIndex = -1;
  double bestDensity = -1.0;
  for (int i = 0; i < numCandidates; i++) {
    double candidateDensity = \
      (computedWidth == -1) ? mCandidates[i].Density(this)
                            : mCandidates[i].Density(computedWidth);
    
    
    
    if (bestIndex == -1 ||
        (bestDensity < displayDensity && candidateDensity > bestDensity) ||
        (candidateDensity >= displayDensity && candidateDensity < bestDensity)) {
      bestIndex = i;
      bestDensity = candidateDensity;
    }
  }

  MOZ_ASSERT(bestIndex >= 0 && bestIndex < numCandidates);

  
  nsresult rv;
  const nsAString& urlStr = mCandidates[bestIndex].URLString();
  nsCOMPtr<nsIURI> candidateURL;
  rv = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(candidateURL),
                                                 urlStr, doc, baseURI);

  mSelectedCandidateURL = NS_SUCCEEDED(rv) ? candidateURL : nullptr;
  mSelectedCandidateIndex = bestIndex;

  return mSelectedCandidateIndex != oldBest;
}

int
ResponsiveImageSelector::GetSelectedCandidateIndex()
{
  SelectImage();

  return mSelectedCandidateIndex;
}

bool
ResponsiveImageSelector::ComputeFinalWidthForCurrentViewport(int32_t *aWidth)
{
  unsigned int numSizes = mSizeQueries.Length();
  nsIDocument* doc = Document();
  nsIPresShell *presShell = doc ? doc->GetShell() : nullptr;
  nsPresContext *pctx = presShell ? presShell->GetPresContext() : nullptr;

  if (!pctx) {
    MOZ_ASSERT(false, "Unable to find presContext for this content");
    return false;
  }

  MOZ_ASSERT(numSizes == mSizeValues.Length(),
             "mSizeValues length differs from mSizeQueries");

  unsigned int i;
  for (i = 0; i < numSizes; i++) {
    if (mSizeQueries[i]->Matches(pctx, nullptr)) {
      break;
    }
  }

  nscoord effectiveWidth;
  if (i == numSizes) {
    
    nsCSSValue defaultWidth(100.0f, eCSSUnit_ViewportWidth);
    effectiveWidth = nsRuleNode::CalcLengthWithInitialFont(pctx,
                                                           defaultWidth);
  } else {
    effectiveWidth = nsRuleNode::CalcLengthWithInitialFont(pctx,
                                                           mSizeValues[i]);
  }

  MOZ_ASSERT(effectiveWidth >= 0);
  *aWidth = nsPresContext::AppUnitsToIntCSSPixels(std::max(effectiveWidth, 0));
  return true;
}

ResponsiveImageCandidate::ResponsiveImageCandidate()
{
  mType = eCandidateType_Invalid;
  mValue.mDensity = 1.0;
}

ResponsiveImageCandidate::ResponsiveImageCandidate(const nsAString& aURLString,
                                                   double aDensity)
  : mURLString(aURLString)
{
  mType = eCandidateType_Density;
  mValue.mDensity = aDensity;
}


void
ResponsiveImageCandidate::SetURLSpec(const nsAString& aURLString)
{
  mURLString = aURLString;
}

void
ResponsiveImageCandidate::SetParameterAsComputedWidth(int32_t aWidth)
{
  mType = eCandidateType_ComputedFromWidth;
  mValue.mWidth = aWidth;
}

void
ResponsiveImageCandidate::SetParameterDefault()
{
  MOZ_ASSERT(mType == eCandidateType_Invalid, "double setting candidate type");

  mType = eCandidateType_Default;
  
  
  mValue.mDensity = 1.0;
}

void
ResponsiveImageCandidate::SetParameterInvalid()
{
  mType = eCandidateType_Invalid;
  
  
  mValue.mDensity = 1.0;
}

void
ResponsiveImageCandidate::SetParameterAsDensity(double aDensity)
{
  MOZ_ASSERT(mType == eCandidateType_Invalid, "double setting candidate type");

  mType = eCandidateType_Density;
  mValue.mDensity = aDensity;
}




struct ResponsiveImageDescriptors {
  ResponsiveImageDescriptors()
    : mInvalid(false) {};

  Maybe<double> mDensity;
  Maybe<int32_t> mWidth;
  
  
  
  Maybe<int32_t> mFutureCompatHeight;
  
  
  bool mInvalid;

  void AddDescriptor(const nsAString& aDescriptor);
  bool Valid();
  
  void FillCandidate(ResponsiveImageCandidate &aCandidate);
};





void
ResponsiveImageDescriptors::AddDescriptor(const nsAString& aDescriptor)
{
  if (aDescriptor.IsEmpty()) {
    return;
  }

  
  nsAString::const_iterator descStart, descType;
  aDescriptor.BeginReading(descStart);
  aDescriptor.EndReading(descType);
  descType--;
  const nsDependentSubstring& valueStr = Substring(descStart, descType);
  if (*descType == char16_t('w')) {
    int32_t possibleWidth;
    
    
    if (ParseInteger(valueStr, possibleWidth) && possibleWidth >= 0) {
      if (possibleWidth != 0 && HTMLPictureElement::IsPictureEnabled() &&
          mWidth.isNothing() && mDensity.isNothing()) {
        mWidth.emplace(possibleWidth);
      } else {
        
        
        mInvalid = true;
      }

      return;
    }
  } else if (*descType == char16_t('h')) {
    int32_t possibleHeight;
    
    
    if (ParseInteger(valueStr, possibleHeight) && possibleHeight >= 0) {
      if (possibleHeight != 0 && mFutureCompatHeight.isNothing() &&
          mDensity.isNothing()) {
        mFutureCompatHeight.emplace(possibleHeight);
      } else {
        
        
        mInvalid = true;
      }

      return;
    }
  } else if (*descType == char16_t('x')) {
    
    
    nsresult rv;
    double possibleDensity = PromiseFlatString(valueStr).ToDouble(&rv);
    if (NS_SUCCEEDED(rv)) {
      if (possibleDensity >= 0.0 &&
          mWidth.isNothing() &&
          mDensity.isNothing() &&
          mFutureCompatHeight.isNothing()) {
        mDensity.emplace(possibleDensity);
      } else {
        
        
        mInvalid = true;
      }

      return;
    }
  }

  
  mInvalid = true;
}

bool
ResponsiveImageDescriptors::Valid()
{
  return !mInvalid && !(mFutureCompatHeight.isSome() && mWidth.isNothing());
}

void
ResponsiveImageDescriptors::FillCandidate(ResponsiveImageCandidate &aCandidate)
{
  if (!Valid()) {
    aCandidate.SetParameterInvalid();
  } else if (mWidth.isSome()) {
    MOZ_ASSERT(mDensity.isNothing()); 

    aCandidate.SetParameterAsComputedWidth(*mWidth);
  } else if (mDensity.isSome()) {
    MOZ_ASSERT(mWidth.isNothing()); 

    aCandidate.SetParameterAsDensity(*mDensity);
  } else {
    
    
    aCandidate.SetParameterAsDensity(1.0);
  }
}

bool
ResponsiveImageCandidate::ConsumeDescriptors(nsAString::const_iterator& aIter,
                                             const nsAString::const_iterator& aIterEnd)
{
  nsAString::const_iterator &iter = aIter;
  const nsAString::const_iterator &end  = aIterEnd;

  bool inParens = false;

  ResponsiveImageDescriptors descriptors;

  
  
  

  
  for (; iter != end && nsContentUtils::IsHTMLWhitespace(*iter); ++iter);

  nsAString::const_iterator currentDescriptor = iter;

  for (;; iter++) {
    if (iter == end) {
      descriptors.AddDescriptor(Substring(currentDescriptor, iter));
      break;
    } else if (inParens) {
      if (*iter == char16_t(')')) {
        inParens = false;
      }
    } else {
      if (*iter == char16_t(',')) {
        
        
        descriptors.AddDescriptor(Substring(currentDescriptor, iter));
        iter++;
        break;
      } else if (nsContentUtils::IsHTMLWhitespace(*iter)) {
        
        
        descriptors.AddDescriptor(Substring(currentDescriptor, iter));
        for (; iter != end && nsContentUtils::IsHTMLWhitespace(*iter); ++iter);
        if (iter == end) {
          break;
        }
        currentDescriptor = iter;
        
        iter--;
      } else if (*iter == char16_t('(')) {
        inParens = true;
      }
    }
  }

  descriptors.FillCandidate(*this);

  return Type() != eCandidateType_Invalid;
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
  } else if (mType == eCandidateType_ComputedFromWidth) {
    return aOther.mValue.mWidth == mValue.mWidth;
  }

  MOZ_ASSERT(false, "Somebody forgot to check for all uses of this enum");
  return false;
}

const nsAString&
ResponsiveImageCandidate::URLString() const
{
  return mURLString;
}

double
ResponsiveImageCandidate::Density(ResponsiveImageSelector *aSelector) const
{
  if (mType == eCandidateType_ComputedFromWidth) {
    int32_t width;
    if (!aSelector->ComputeFinalWidthForCurrentViewport(&width)) {
      return 1.0;
    }
    return Density(width);
  }

  
  MOZ_ASSERT(mType == eCandidateType_Default || mType == eCandidateType_Density,
             "unhandled candidate type");
  return Density(-1);
}

double
ResponsiveImageCandidate::Density(int32_t aMatchingWidth) const
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
  } else if (mType == eCandidateType_ComputedFromWidth) {
    if (aMatchingWidth <= 0) {
      MOZ_ASSERT(false, "0 or negative matching width is invalid per spec");
      return 1.0;
    }
    double density = double(mValue.mWidth) / double(aMatchingWidth);
    MOZ_ASSERT(density > 0.0);
    return density;
  }

  MOZ_ASSERT(false, "Unknown candidate type");
  return 1.0;
}

bool
ResponsiveImageCandidate::IsComputedFromWidth() const
{
  if (mType == eCandidateType_ComputedFromWidth) {
    return true;
  }

  MOZ_ASSERT(mType == eCandidateType_Default || mType == eCandidateType_Density,
             "Unknown candidate type");
  return false;
}

} 
} 
