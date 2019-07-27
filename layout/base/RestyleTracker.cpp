









#include "RestyleTracker.h"

#include "GeckoProfiler.h"
#include "nsFrameManager.h"
#include "nsIDocument.h"
#include "nsStyleChangeList.h"
#include "RestyleManager.h"
#include "RestyleTrackerInlines.h"
#include "nsTransitionManager.h"

namespace mozilla {

#ifdef RESTYLE_LOGGING
static nsCString
GetDocumentURI(nsIDocument* aDocument)
{
  nsCString result;
  nsString url;
  aDocument->GetDocumentURI(url);
  result.Append(NS_ConvertUTF16toUTF8(url).get());
  return result;
}

static nsCString
FrameTagToString(dom::Element* aElement)
{
  nsCString result;
  nsIFrame* frame = aElement->GetPrimaryFrame();
  if (frame) {
    nsFrame::ListTag(result, frame);
  } else {
    nsAutoString buf;
    aElement->NodeInfo()->NameAtom()->ToString(buf);
    result.AppendPrintf("(%s@%p)", NS_ConvertUTF16toUTF8(buf).get(), aElement);
  }
  return result;
}
#endif

inline nsIDocument*
RestyleTracker::Document() const {
  return mRestyleManager->PresContext()->Document();
}

#define RESTYLE_ARRAY_STACKSIZE 128

struct LaterSiblingCollector {
  RestyleTracker* tracker;
  nsTArray< nsRefPtr<dom::Element> >* elements;
};

static PLDHashOperator
CollectLaterSiblings(nsISupports* aElement,
                     nsAutoPtr<RestyleTracker::RestyleData>& aData,
                     void* aSiblingCollector)
{
  dom::Element* element =
    static_cast<dom::Element*>(aElement);
  LaterSiblingCollector* collector =
    static_cast<LaterSiblingCollector*>(aSiblingCollector);
  
  
  
  
  if (element->GetCrossShadowCurrentDoc() == collector->tracker->Document() &&
      element->HasFlag(collector->tracker->RestyleBit()) &&
      (aData->mRestyleHint & eRestyle_LaterSiblings)) {
    collector->elements->AppendElement(element);
  }

  return PL_DHASH_NEXT;
}

struct RestyleEnumerateData : RestyleTracker::Hints {
  nsRefPtr<dom::Element> mElement;
#ifdef MOZ_ENABLE_PROFILER_SPS
  UniquePtr<ProfilerBacktrace> mBacktrace;
#endif
};

struct RestyleCollector {
  RestyleTracker* tracker;
  RestyleEnumerateData** restyleArrayPtr;
#ifdef RESTYLE_LOGGING
  uint32_t count;
#endif
};

static PLDHashOperator
CollectRestyles(nsISupports* aElement,
                nsAutoPtr<RestyleTracker::RestyleData>& aData,
                void* aRestyleCollector)
{
  dom::Element* element =
    static_cast<dom::Element*>(aElement);
  RestyleCollector* collector =
    static_cast<RestyleCollector*>(aRestyleCollector);
  
  
  
  
  if (element->GetCrossShadowCurrentDoc() != collector->tracker->Document() ||
      !element->HasFlag(collector->tracker->RestyleBit())) {
    LOG_RESTYLE_IF(collector->tracker, true,
                   "skipping pending restyle %s, already restyled or no longer "
                   "in the document", FrameTagToString(element).get());
    return PL_DHASH_NEXT;
  }

  NS_ASSERTION(!element->HasFlag(collector->tracker->RootBit()) ||
               
               (element->GetFlattenedTreeParent() &&
                (!element->GetFlattenedTreeParent()->GetPrimaryFrame() ||
                 element->GetFlattenedTreeParent()->GetPrimaryFrame()->IsLeaf() ||
                 element->GetCurrentDoc()->GetShell()->FrameManager()
                   ->GetDisplayContentsStyleFor(element))) ||
               
               
               
               
               
               
               (aData->mChangeHint & nsChangeHint_ReconstructFrame),
               "Why did this not get handled while processing mRestyleRoots?");

  
  
  element->UnsetFlags(collector->tracker->RestyleBit() |
                      collector->tracker->RootBit());

  RestyleEnumerateData** restyleArrayPtr = collector->restyleArrayPtr;
  RestyleEnumerateData* currentRestyle = *restyleArrayPtr;
  currentRestyle->mElement = element;
  currentRestyle->mRestyleHint = aData->mRestyleHint;
  currentRestyle->mChangeHint = aData->mChangeHint;
#ifdef MOZ_ENABLE_PROFILER_SPS
  currentRestyle->mBacktrace = Move(aData->mBacktrace);
#endif

#ifdef RESTYLE_LOGGING
  collector->count++;
#endif

  
  *restyleArrayPtr = currentRestyle + 1;

  return PL_DHASH_NEXT;
}

inline void
RestyleTracker::ProcessOneRestyle(Element* aElement,
                                  nsRestyleHint aRestyleHint,
                                  nsChangeHint aChangeHint)
{
  NS_PRECONDITION((aRestyleHint & eRestyle_LaterSiblings) == 0,
                  "Someone should have handled this before calling us");
  NS_PRECONDITION(Document(), "Must have a document");
  NS_PRECONDITION(aElement->GetCrossShadowCurrentDoc() == Document(),
                  "Element has unexpected document");

  LOG_RESTYLE("aRestyleHint = %s, aChangeHint = %s",
              RestyleManager::RestyleHintToString(aRestyleHint).get(),
              RestyleManager::ChangeHintToString(aChangeHint).get());

  nsIFrame* primaryFrame = aElement->GetPrimaryFrame();

  if (aRestyleHint & ~eRestyle_LaterSiblings) {
#ifdef RESTYLE_LOGGING
    if (ShouldLogRestyle() && primaryFrame &&
        RestyleManager::StructsToLog() != 0) {
      LOG_RESTYLE("style context tree before restyle:");
      LOG_RESTYLE_INDENT();
      primaryFrame->StyleContext()->LogStyleContextTree(
          LoggingDepth(), RestyleManager::StructsToLog());
    }
#endif
    mRestyleManager->RestyleElement(aElement, primaryFrame, aChangeHint,
                                    *this, aRestyleHint);
  } else if (aChangeHint &&
             (primaryFrame ||
              (aChangeHint & nsChangeHint_ReconstructFrame))) {
    
    nsStyleChangeList changeList;
    changeList.AppendChange(primaryFrame, aElement, aChangeHint);
    mRestyleManager->ProcessRestyledFrames(changeList);
  }
}

void
RestyleTracker::DoProcessRestyles()
{
  PROFILER_LABEL("RestyleTracker", "ProcessRestyles",
    js::ProfileEntry::Category::CSS);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  {
    RestyleManager::ReframingStyleContexts
      reframingStyleContexts(mRestyleManager);

    mRestyleManager->BeginProcessingRestyles(*this);

    LOG_RESTYLE("Processing %d pending %srestyles with %d restyle roots for %s",
                mPendingRestyles.Count(),
                mRestyleManager->PresContext()->TransitionManager()->
                  InAnimationOnlyStyleUpdate()
                  ? (const char*) "animation " : (const char*) "",
                static_cast<int>(mRestyleRoots.Length()),
                GetDocumentURI(Document()).get());
    LOG_RESTYLE_INDENT();

    
    while (mPendingRestyles.Count()) {
      if (mHaveLaterSiblingRestyles) {
        
        nsAutoTArray<nsRefPtr<Element>, RESTYLE_ARRAY_STACKSIZE> laterSiblingArr;
        LaterSiblingCollector siblingCollector = { this, &laterSiblingArr };
        mPendingRestyles.Enumerate(CollectLaterSiblings, &siblingCollector);
        for (uint32_t i = 0; i < laterSiblingArr.Length(); ++i) {
          Element* element = laterSiblingArr[i];
          for (nsIContent* sibling = element->GetNextSibling();
               sibling;
               sibling = sibling->GetNextSibling()) {
            if (sibling->IsElement()) {
              LOG_RESTYLE("adding pending restyle for %s due to "
                          "eRestyle_LaterSiblings hint on %s",
                          FrameTagToString(sibling->AsElement()).get(),
                          FrameTagToString(element->AsElement()).get());
              if (AddPendingRestyle(sibling->AsElement(), eRestyle_Subtree,
                                    NS_STYLE_HINT_NONE)) {
                  
                  
                break;
              }
            }
          }
        }

        
        for (uint32_t i = 0; i < laterSiblingArr.Length(); ++i) {
          Element* element = laterSiblingArr[i];
          NS_ASSERTION(element->HasFlag(RestyleBit()), "How did that happen?");
          RestyleData* data;
#ifdef DEBUG
          bool found =
#endif
            mPendingRestyles.Get(element, &data);
          NS_ASSERTION(found, "Where did our entry go?");
          data->mRestyleHint =
            nsRestyleHint(data->mRestyleHint & ~eRestyle_LaterSiblings);
        }

        LOG_RESTYLE("%d pending restyles after expanding out "
                    "eRestyle_LaterSiblings", mPendingRestyles.Count());

        mHaveLaterSiblingRestyles = false;
      }

      uint32_t rootCount;
      while ((rootCount = mRestyleRoots.Length())) {
        
        
        
        nsRefPtr<Element> element;
        element.swap(mRestyleRoots[rootCount - 1]);
        mRestyleRoots.RemoveElementAt(rootCount - 1);

        LOG_RESTYLE("processing style root %s at index %d",
                    FrameTagToString(element).get(), rootCount - 1);
        LOG_RESTYLE_INDENT();

        
        
        
        if (element->GetCrossShadowCurrentDoc() != Document()) {
          
          
          LOG_RESTYLE("skipping, no longer in the document");
          continue;
        }

        nsAutoPtr<RestyleData> data;
        if (!GetRestyleData(element, data)) {
          LOG_RESTYLE("skipping, already restyled");
          continue;
        }

        Maybe<GeckoProfilerTracingRAII> profilerRAII;
#ifdef MOZ_ENABLE_PROFILER_SPS
        if (profiler_feature_active("restyle")) {
          profilerRAII.emplace("Paint", "Styles", Move(data->mBacktrace));
        }
#endif
        ProcessOneRestyle(element, data->mRestyleHint, data->mChangeHint);
        AddRestyleRootsIfAwaitingRestyle(data->mDescendants);
      }

      if (mHaveLaterSiblingRestyles) {
        
        continue;
      }

      
      
      
      
      
      nsAutoTArray<RestyleEnumerateData, RESTYLE_ARRAY_STACKSIZE> restyleArr;
      RestyleEnumerateData* restylesToProcess =
        restyleArr.AppendElements(mPendingRestyles.Count());
      if (restylesToProcess) {
        RestyleEnumerateData* lastRestyle = restylesToProcess;
        RestyleCollector collector = { this, &lastRestyle };
        mPendingRestyles.Enumerate(CollectRestyles, &collector);

        
        mPendingRestyles.Clear();

#ifdef RESTYLE_LOGGING
        uint32_t index = 0;
#endif
        for (RestyleEnumerateData* currentRestyle = restylesToProcess;
             currentRestyle != lastRestyle;
             ++currentRestyle) {
          LOG_RESTYLE("processing pending restyle %s at index %d/%d",
                      FrameTagToString(currentRestyle->mElement).get(),
                      index++, collector.count);
          LOG_RESTYLE_INDENT();

          Maybe<GeckoProfilerTracingRAII> profilerRAII;
#ifdef MOZ_ENABLE_PROFILER_SPS
          if (profiler_feature_active("restyle")) {
            profilerRAII.emplace("Paint", "Styles", Move(currentRestyle->mBacktrace));
          }
#endif
          ProcessOneRestyle(currentRestyle->mElement,
                            currentRestyle->mRestyleHint,
                            currentRestyle->mChangeHint);
        }
      }
    }
  }

  mRestyleManager->EndProcessingRestyles();
}

bool
RestyleTracker::GetRestyleData(Element* aElement, nsAutoPtr<RestyleData>& aData)
{
  NS_PRECONDITION(aElement->GetCrossShadowCurrentDoc() == Document(),
                  "Unexpected document; this will lead to incorrect behavior!");

  if (!aElement->HasFlag(RestyleBit())) {
    NS_ASSERTION(!aElement->HasFlag(RootBit()), "Bogus root bit?");
    return false;
  }

  mPendingRestyles.RemoveAndForget(aElement, aData);
  NS_ASSERTION(aData.get(), "Must have data if restyle bit is set");

  if (aData->mRestyleHint & eRestyle_LaterSiblings) {
    
    
    
    
    NS_ASSERTION(aData->mDescendants.IsEmpty(),
                 "expected descendants to be handled by now");
    RestyleData* newData = new RestyleData;
    newData->mChangeHint = nsChangeHint(0);
    newData->mRestyleHint = eRestyle_LaterSiblings;
    mPendingRestyles.Put(aElement, newData);
    aElement->UnsetFlags(RootBit());
    aData->mRestyleHint =
      nsRestyleHint(aData->mRestyleHint & ~eRestyle_LaterSiblings);
  } else {
    aElement->UnsetFlags(mRestyleBits);
  }

  return true;
}

void
RestyleTracker::AddRestyleRootsIfAwaitingRestyle(
                                   const nsTArray<nsRefPtr<Element>>& aElements)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  for (size_t i = 0; i < aElements.Length(); i++) {
    Element* element = aElements[i];
    if (element->HasFlag(RestyleBit())) {
      mRestyleRoots.AppendElement(element);
    }
  }
}

} 
