





#include "base/basictypes.h"
#include "mozilla/Util.h"

#include "nsLayoutUtils.h"
#include "nsIFormControlFrame.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLElement.h"
#include "nsFrameList.h"
#include "nsGkAtoms.h"
#include "nsIAtom.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSColorUtils.h"
#include "nsIView.h"
#include "nsPlaceholderFrame.h"
#include "nsIScrollableFrame.h"
#include "nsCSSFrameConstructor.h"
#include "nsIDOMEvent.h"
#include "nsGUIEvent.h"
#include "nsDisplayList.h"
#include "nsRegion.h"
#include "nsFrameManager.h"
#include "nsBlockFrame.h"
#include "nsBidiPresUtils.h"
#include "imgIContainer.h"
#include "gfxRect.h"
#include "gfxContext.h"
#include "gfxFont.h"
#include "nsRenderingContext.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsCSSRendering.h"
#include "nsContentUtils.h"
#include "nsThemeConstants.h"
#include "nsPIDOMWindow.h"
#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIWidget.h"
#include "gfxMatrix.h"
#include "gfxPoint3D.h"
#include "gfxTypes.h"
#include "gfxUserFontSet.h"
#include "nsTArray.h"
#include "nsHTMLCanvasElement.h"
#include "nsICanvasRenderingContextInternal.h"
#include "gfxPlatform.h"
#include "nsClientRect.h"
#ifdef MOZ_MEDIA
#include "nsHTMLVideoElement.h"
#endif
#include "nsHTMLImageElement.h"
#include "imgIRequest.h"
#include "nsIImageLoadingContent.h"
#include "nsCOMPtr.h"
#include "nsListControlFrame.h"
#include "ImageLayers.h"
#include "mozilla/arm.h"
#include "mozilla/dom/Element.h"
#include "nsCanvasFrame.h"
#include "gfxDrawable.h"
#include "gfxUtils.h"
#include "nsDataHashtable.h"
#include "nsTextFrame.h"
#include "nsFontFaceList.h"
#include "nsFontInflationData.h"
#include "CompositorParent.h"
#include "nsSVGUtils.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGForeignObjectFrame.h"
#include "nsSVGOuterSVGFrame.h"

#include "mozilla/Preferences.h"

#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#endif

#include "sampler.h"
#include "nsAnimationManager.h"
#include "nsTransitionManager.h"

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::dom;
using namespace mozilla::layout;

#ifdef DEBUG

bool nsLayoutUtils::gPreventAssertInCompareTreePosition = false;
#endif 

typedef gfxPattern::GraphicsFilter GraphicsFilter;
typedef FrameMetrics::ViewID ViewID;

 PRUint32 nsLayoutUtils::sFontSizeInflationEmPerLine;
 PRUint32 nsLayoutUtils::sFontSizeInflationMinTwips;
 PRUint32 nsLayoutUtils::sFontSizeInflationLineThreshold;

static ViewID sScrollIdCounter = FrameMetrics::START_SCROLL_ID;

typedef nsDataHashtable<nsUint64HashKey, nsIContent*> ContentMap;
static ContentMap* sContentMap = NULL;
static ContentMap& GetContentMap() {
  if (!sContentMap) {
    sContentMap = new ContentMap();
    sContentMap->Init();
  }
  return *sContentMap;
}

bool
nsLayoutUtils::HasAnimationsForCompositor(nsIContent* aContent,
                                          nsCSSProperty aProperty)
{
  if (!aContent->MayHaveAnimations())
    return false;
  ElementAnimations* animations =
    static_cast<ElementAnimations*>(aContent->GetProperty(nsGkAtoms::animationsProperty));
  if (animations) {
    bool propertyMatches = animations->HasAnimationOfProperty(aProperty);
    if (propertyMatches && animations->CanPerformOnCompositorThread()) {
      return true;
    }
  }

  ElementTransitions* transitions =
    static_cast<ElementTransitions*>(aContent->GetProperty(nsGkAtoms::transitionsProperty));
  if (transitions) {
    bool propertyMatches = transitions->HasTransitionOfProperty(aProperty);
    if (propertyMatches && transitions->CanPerformOnCompositorThread()) {
      return true;
    }
  }

  return false;
}

bool
nsLayoutUtils::Are3DTransformsEnabled()
{
  static bool s3DTransformsEnabled;
  static bool s3DTransformPrefCached = false;

  if (!s3DTransformPrefCached) {
    s3DTransformPrefCached = true;
    mozilla::Preferences::AddBoolVarCache(&s3DTransformsEnabled,
                                          "layout.3d-transforms.enabled");
  }

  return s3DTransformsEnabled;
}

bool
nsLayoutUtils::AreOpacityAnimationsEnabled()
{
  static bool sAreOpacityAnimationsEnabled;
  static bool sOpacityPrefCached = false;

  if (!sOpacityPrefCached) {
    sOpacityPrefCached = true;
    Preferences::AddBoolVarCache(&sAreOpacityAnimationsEnabled,
                                 "layers.offmainthreadcomposition.animate-opacity");
  }

  return sAreOpacityAnimationsEnabled && CompositorParent::CompositorLoop();
}

bool
nsLayoutUtils::AreTransformAnimationsEnabled()
{
  static bool sAreTransformAnimationsEnabled;
  static bool sTransformPrefCached = false;

  if (!sTransformPrefCached) {
    sTransformPrefCached = true;
    Preferences::AddBoolVarCache(&sAreTransformAnimationsEnabled,
                                 "layers.offmainthreadcomposition.animate-transform");
  }

  return sAreTransformAnimationsEnabled && CompositorParent::CompositorLoop();
}

bool
nsLayoutUtils::IsAnimationLoggingEnabled()
{
  static bool sShouldLog;
  static bool sShouldLogPrefCached;

  if (!sShouldLogPrefCached) {
    sShouldLogPrefCached = true;
    Preferences::AddBoolVarCache(&sShouldLog,
                                 "layers.offmainthreadcomposition.log-animations");
  }

  return sShouldLog;
}

bool
nsLayoutUtils::UseBackgroundNearestFiltering()
{
  static bool sUseBackgroundNearestFilteringEnabled;
  static bool sUseBackgroundNearestFilteringPrefInitialised = false;

  if (!sUseBackgroundNearestFilteringPrefInitialised) {
    sUseBackgroundNearestFilteringPrefInitialised = true;
    sUseBackgroundNearestFilteringEnabled = mozilla::Preferences::GetBool("gfx.filter.nearest.force-enabled", false);
  }

  return sUseBackgroundNearestFilteringEnabled;
}

bool
nsLayoutUtils::GPUImageScalingEnabled()
{
  static bool sGPUImageScalingEnabled;
  static bool sGPUImageScalingPrefInitialised = false;

  if (!sGPUImageScalingPrefInitialised) {
    sGPUImageScalingPrefInitialised = true;
    sGPUImageScalingEnabled = mozilla::Preferences::GetBool("layout.gpu-image-scaling.enabled", false);
  }

  return sGPUImageScalingEnabled;
}

void
nsLayoutUtils::UnionChildOverflow(nsIFrame* aFrame,
                                  nsOverflowAreas& aOverflowAreas)
{
  
  const nsIFrame::ChildListIDs skip(nsIFrame::kPopupList |
                                    nsIFrame::kSelectPopupList);
  for (nsIFrame::ChildListIterator childLists(aFrame);
       !childLists.IsDone(); childLists.Next()) {
    if (skip.Contains(childLists.CurrentID())) {
      continue;
    }

    nsFrameList children = childLists.CurrentList();
    for (nsFrameList::Enumerator e(children); !e.AtEnd(); e.Next()) {
      nsIFrame* child = e.get();
      nsOverflowAreas childOverflow =
        child->GetOverflowAreas() + child->GetPosition();
      aOverflowAreas.UnionWith(childOverflow);
    }
  }
}

static void DestroyViewID(void* aObject, nsIAtom* aPropertyName,
                          void* aPropertyValue, void* aData)
{
  ViewID* id = static_cast<ViewID*>(aPropertyValue);
  GetContentMap().Remove(*id);
  delete id;
}





ViewID
nsLayoutUtils::FindIDFor(nsIContent* aContent)
{
  ViewID scrollId;

  void* scrollIdProperty = aContent->GetProperty(nsGkAtoms::RemoteId);
  if (scrollIdProperty) {
    scrollId = *static_cast<ViewID*>(scrollIdProperty);
  } else {
    scrollId = sScrollIdCounter++;
    aContent->SetProperty(nsGkAtoms::RemoteId, new ViewID(scrollId),
                          DestroyViewID);
    GetContentMap().Put(scrollId, aContent);
  }

  return scrollId;
}

nsIContent*
nsLayoutUtils::FindContentFor(ViewID aId)
{
  NS_ABORT_IF_FALSE(aId != FrameMetrics::NULL_SCROLL_ID &&
                    aId != FrameMetrics::ROOT_SCROLL_ID,
                    "Cannot find a content element in map for null or root IDs.");
  nsIContent* content;
  bool exists = GetContentMap().Get(aId, &content);

  if (exists) {
    return content;
  } else {
    return nullptr;
  }
}

bool
nsLayoutUtils::GetDisplayPort(nsIContent* aContent, nsRect *aResult)
{
  void* property = aContent->GetProperty(nsGkAtoms::DisplayPort);
  if (!property) {
    return false;
  }

  if (aResult) {
    *aResult = *static_cast<nsRect*>(property);
  }
  return true;
}

nsIFrame*
nsLayoutUtils::GetLastContinuationWithChild(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");
  aFrame = aFrame->GetLastContinuation();
  while (!aFrame->GetFirstPrincipalChild() &&
         aFrame->GetPrevContinuation()) {
    aFrame = aFrame->GetPrevContinuation();
  }
  return aFrame;
}








static nsIFrame*
GetFirstChildFrame(nsIFrame*       aFrame,
                   nsIContent*     aContent)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");

  
  nsIFrame* childFrame = aFrame->GetFirstPrincipalChild();

  
  
  
  if (childFrame &&
      childFrame->IsPseudoFrame(aContent) &&
      !childFrame->IsGeneratedContentFrame()) {
    return GetFirstChildFrame(childFrame, aContent);
  }

  return childFrame;
}








static nsIFrame*
GetLastChildFrame(nsIFrame*       aFrame,
                  nsIContent*     aContent)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");

  
  nsIFrame* lastParentContinuation =
    nsLayoutUtils::GetLastContinuationWithChild(aFrame);
  nsIFrame* lastChildFrame =
    lastParentContinuation->GetLastChild(nsIFrame::kPrincipalList);
  if (lastChildFrame) {
    
    
    lastChildFrame = lastChildFrame->GetFirstContinuation();

    
    
    
    if (lastChildFrame &&
        lastChildFrame->IsPseudoFrame(aContent) &&
        !lastChildFrame->IsGeneratedContentFrame()) {
      return GetLastChildFrame(lastChildFrame, aContent);
    }

    return lastChildFrame;
  }

  return nullptr;
}


nsIFrame::ChildListID
nsLayoutUtils::GetChildListNameFor(nsIFrame* aChildFrame)
{
  nsIFrame::ChildListID id = nsIFrame::kPrincipalList;

  if (aChildFrame->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) {
    nsIFrame* pif = aChildFrame->GetPrevInFlow();
    if (pif->GetParent() == aChildFrame->GetParent()) {
      id = nsIFrame::kExcessOverflowContainersList;
    }
    else {
      id = nsIFrame::kOverflowContainersList;
    }
  }
  
  else if (aChildFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
    
    const nsStyleDisplay* disp = aChildFrame->GetStyleDisplay();

    if (NS_STYLE_POSITION_ABSOLUTE == disp->mPosition) {
      id = nsIFrame::kAbsoluteList;
    } else if (NS_STYLE_POSITION_FIXED == disp->mPosition) {
      if (nsLayoutUtils::IsReallyFixedPos(aChildFrame)) {
        id = nsIFrame::kFixedList;
      } else {
        id = nsIFrame::kAbsoluteList;
      }
#ifdef MOZ_XUL
    } else if (NS_STYLE_DISPLAY_POPUP == disp->mDisplay) {
      
#ifdef DEBUG
      nsIFrame* parent = aChildFrame->GetParent();
      NS_ASSERTION(parent && parent->GetType() == nsGkAtoms::popupSetFrame,
                   "Unexpected parent");
#endif 

      
      
      
      
      return nsIFrame::kPopupList;
#endif
    } else {
      NS_ASSERTION(aChildFrame->IsFloating(),
                   "not a floated frame");
      id = nsIFrame::kFloatList;
    }

  } else {
    nsIAtom* childType = aChildFrame->GetType();
    if (nsGkAtoms::menuPopupFrame == childType) {
      nsIFrame* parent = aChildFrame->GetParent();
      nsIFrame* firstPopup = (parent)
                             ? parent->GetFirstChild(nsIFrame::kPopupList)
                             : nullptr;
      NS_ASSERTION(!firstPopup || !firstPopup->GetNextSibling(),
                   "We assume popupList only has one child, but it has more.");
      id = firstPopup == aChildFrame
             ? nsIFrame::kPopupList
             : nsIFrame::kPrincipalList;
    } else if (nsGkAtoms::tableColGroupFrame == childType) {
      id = nsIFrame::kColGroupList;
    } else if (nsGkAtoms::tableCaptionFrame == aChildFrame->GetType()) {
      id = nsIFrame::kCaptionList;
    } else {
      id = nsIFrame::kPrincipalList;
    }
  }

#ifdef DEBUG
  
  
  nsIFrame* parent = aChildFrame->GetParent();
  bool found = parent->GetChildList(id).ContainsFrame(aChildFrame);
  if (!found) {
    if (!(aChildFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
      found = parent->GetChildList(nsIFrame::kOverflowList)
                .ContainsFrame(aChildFrame);
    }
    else if (aChildFrame->IsFloating()) {
      found = parent->GetChildList(nsIFrame::kOverflowOutOfFlowList)
                .ContainsFrame(aChildFrame);
    }
    
    NS_POSTCONDITION(found, "not in child list");
  }
#endif

  return id;
}


nsIFrame*
nsLayoutUtils::GetBeforeFrame(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");
  NS_ASSERTION(!aFrame->GetPrevContinuation(),
               "aFrame must be first continuation");

  nsIFrame* firstFrame = GetFirstChildFrame(aFrame, aFrame->GetContent());

  if (firstFrame && IsGeneratedContentFor(nullptr, firstFrame,
                                          nsCSSPseudoElements::before)) {
    return firstFrame;
  }

  return nullptr;
}


nsIFrame*
nsLayoutUtils::GetAfterFrame(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");

  nsIFrame* lastFrame = GetLastChildFrame(aFrame, aFrame->GetContent());

  if (lastFrame && IsGeneratedContentFor(nullptr, lastFrame,
                                         nsCSSPseudoElements::after)) {
    return lastFrame;
  }

  return nullptr;
}


nsIFrame*
nsLayoutUtils::GetClosestFrameOfType(nsIFrame* aFrame, nsIAtom* aFrameType)
{
  for (nsIFrame* frame = aFrame; frame; frame = frame->GetParent()) {
    if (frame->GetType() == aFrameType) {
      return frame;
    }
  }
  return nullptr;
}


nsIFrame*
nsLayoutUtils::GetStyleFrame(nsIFrame* aFrame)
{
  if (aFrame->GetType() == nsGkAtoms::tableOuterFrame) {
    nsIFrame* inner = aFrame->GetFirstPrincipalChild();
    NS_ASSERTION(inner, "Outer table must have an inner");
    return inner;
  }

  return aFrame;
}

nsIFrame*
nsLayoutUtils::GetFloatFromPlaceholder(nsIFrame* aFrame) {
  NS_ASSERTION(nsGkAtoms::placeholderFrame == aFrame->GetType(),
               "Must have a placeholder here");
  if (aFrame->GetStateBits() & PLACEHOLDER_FOR_FLOAT) {
    nsIFrame *outOfFlowFrame =
      nsPlaceholderFrame::GetRealFrameForPlaceholder(aFrame);
    NS_ASSERTION(outOfFlowFrame->IsFloating(),
                 "How did that happen?");
    return outOfFlowFrame;
  }

  return nullptr;
}


bool
nsLayoutUtils::IsGeneratedContentFor(nsIContent* aContent,
                                     nsIFrame* aFrame,
                                     nsIAtom* aPseudoElement)
{
  NS_PRECONDITION(aFrame, "Must have a frame");
  NS_PRECONDITION(aPseudoElement, "Must have a pseudo name");

  if (!aFrame->IsGeneratedContentFrame()) {
    return false;
  }
  nsIFrame* parent = aFrame->GetParent();
  NS_ASSERTION(parent, "Generated content can't be root frame");
  if (parent->IsGeneratedContentFrame()) {
    
    return false;
  }

  if (aContent && parent->GetContent() != aContent) {
    return false;
  }

  return (aFrame->GetContent()->Tag() == nsGkAtoms::mozgeneratedcontentbefore) ==
    (aPseudoElement == nsCSSPseudoElements::before);
}


nsIFrame*
nsLayoutUtils::GetCrossDocParentFrame(const nsIFrame* aFrame,
                                      nsPoint* aExtraOffset)
{
  nsIFrame* p = aFrame->GetParent();
  if (p)
    return p;

  nsIView* v = aFrame->GetView();
  if (!v)
    return nullptr;
  v = v->GetParent(); 
  if (!v)
    return nullptr;
  if (aExtraOffset) {
    *aExtraOffset += v->GetPosition();
  }
  v = v->GetParent(); 
  return v ? v->GetFrame() : nullptr;
}


bool
nsLayoutUtils::IsProperAncestorFrameCrossDoc(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                             nsIFrame* aCommonAncestor)
{
  if (aFrame == aCommonAncestor)
    return false;

  nsIFrame* parentFrame = GetCrossDocParentFrame(aFrame);

  while (parentFrame != aCommonAncestor) {
    if (parentFrame == aAncestorFrame)
      return true;

    parentFrame = GetCrossDocParentFrame(parentFrame);
  }

  return false;
}


bool
nsLayoutUtils::IsAncestorFrameCrossDoc(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                       nsIFrame* aCommonAncestor)
{
  if (aFrame == aAncestorFrame)
    return true;
  return IsProperAncestorFrameCrossDoc(aAncestorFrame, aFrame, aCommonAncestor);
}


bool
nsLayoutUtils::IsProperAncestorFrame(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                     nsIFrame* aCommonAncestor)
{
  if (aFrame == aCommonAncestor) {
    return false;
  }

  nsIFrame* parentFrame = aFrame->GetParent();

  while (parentFrame != aCommonAncestor) {
    if (parentFrame == aAncestorFrame) {
      return true;
    }

    parentFrame = parentFrame->GetParent();
  }

  return false;
}


PRInt32
nsLayoutUtils::DoCompareTreePosition(nsIContent* aContent1,
                                     nsIContent* aContent2,
                                     PRInt32 aIf1Ancestor,
                                     PRInt32 aIf2Ancestor,
                                     const nsIContent* aCommonAncestor)
{
  NS_PRECONDITION(aContent1, "aContent1 must not be null");
  NS_PRECONDITION(aContent2, "aContent2 must not be null");

  nsAutoTArray<nsINode*, 32> content1Ancestors;
  nsINode* c1;
  for (c1 = aContent1; c1 && c1 != aCommonAncestor; c1 = c1->GetNodeParent()) {
    content1Ancestors.AppendElement(c1);
  }
  if (!c1 && aCommonAncestor) {
    
    
    aCommonAncestor = nullptr;
  }

  nsAutoTArray<nsINode*, 32> content2Ancestors;
  nsINode* c2;
  for (c2 = aContent2; c2 && c2 != aCommonAncestor; c2 = c2->GetNodeParent()) {
    content2Ancestors.AppendElement(c2);
  }
  if (!c2 && aCommonAncestor) {
    
    
    return DoCompareTreePosition(aContent1, aContent2,
                                 aIf1Ancestor, aIf2Ancestor, nullptr);
  }

  int last1 = content1Ancestors.Length() - 1;
  int last2 = content2Ancestors.Length() - 1;
  nsINode* content1Ancestor = nullptr;
  nsINode* content2Ancestor = nullptr;
  while (last1 >= 0 && last2 >= 0
         && ((content1Ancestor = content1Ancestors.ElementAt(last1)) ==
             (content2Ancestor = content2Ancestors.ElementAt(last2)))) {
    last1--;
    last2--;
  }

  if (last1 < 0) {
    if (last2 < 0) {
      NS_ASSERTION(aContent1 == aContent2, "internal error?");
      return 0;
    }
    
    return aIf1Ancestor;
  }

  if (last2 < 0) {
    
    return aIf2Ancestor;
  }

  
  nsINode* parent = content1Ancestor->GetNodeParent();
#ifdef DEBUG
  
  NS_ASSERTION(gPreventAssertInCompareTreePosition || parent,
               "no common ancestor at all???");
#endif 
  if (!parent) { 
    return 0;
  }

  PRInt32 index1 = parent->IndexOf(content1Ancestor);
  PRInt32 index2 = parent->IndexOf(content2Ancestor);
  if (index1 < 0 || index2 < 0) {
    
    return 0;
  }

  return index1 - index2;
}

static nsIFrame* FillAncestors(nsIFrame* aFrame,
                               nsIFrame* aStopAtAncestor,
                               nsTArray<nsIFrame*>* aAncestors)
{
  while (aFrame && aFrame != aStopAtAncestor) {
    aAncestors->AppendElement(aFrame);
    aFrame = nsLayoutUtils::GetParentOrPlaceholderFor(aFrame);
  }
  return aFrame;
}


static bool IsFrameAfter(nsIFrame* aFrame1, nsIFrame* aFrame2)
{
  nsIFrame* f = aFrame2;
  do {
    f = f->GetNextSibling();
    if (f == aFrame1)
      return true;
  } while (f);
  return false;
}


PRInt32
nsLayoutUtils::DoCompareTreePosition(nsIFrame* aFrame1,
                                     nsIFrame* aFrame2,
                                     PRInt32 aIf1Ancestor,
                                     PRInt32 aIf2Ancestor,
                                     nsIFrame* aCommonAncestor)
{
  NS_PRECONDITION(aFrame1, "aFrame1 must not be null");
  NS_PRECONDITION(aFrame2, "aFrame2 must not be null");

  nsPresContext* presContext = aFrame1->PresContext();
  if (presContext != aFrame2->PresContext()) {
    NS_ERROR("no common ancestor at all, different documents");
    return 0;
  }

  nsAutoTArray<nsIFrame*,20> frame1Ancestors;
  if (!FillAncestors(aFrame1, aCommonAncestor, &frame1Ancestors)) {
    
    
    aCommonAncestor = nullptr;
  }

  nsAutoTArray<nsIFrame*,20> frame2Ancestors;
  if (!FillAncestors(aFrame2, aCommonAncestor, &frame2Ancestors) &&
      aCommonAncestor) {
    
    
    return DoCompareTreePosition(aFrame1, aFrame2,
                                 aIf1Ancestor, aIf2Ancestor, nullptr);
  }

  PRInt32 last1 = PRInt32(frame1Ancestors.Length()) - 1;
  PRInt32 last2 = PRInt32(frame2Ancestors.Length()) - 1;
  while (last1 >= 0 && last2 >= 0 &&
         frame1Ancestors[last1] == frame2Ancestors[last2]) {
    last1--;
    last2--;
  }

  if (last1 < 0) {
    if (last2 < 0) {
      NS_ASSERTION(aFrame1 == aFrame2, "internal error?");
      return 0;
    }
    
    return aIf1Ancestor;
  }

  if (last2 < 0) {
    
    return aIf2Ancestor;
  }

  nsIFrame* ancestor1 = frame1Ancestors[last1];
  nsIFrame* ancestor2 = frame2Ancestors[last2];
  
  if (IsFrameAfter(ancestor2, ancestor1))
    return -1;
  if (IsFrameAfter(ancestor1, ancestor2))
    return 1;
  NS_WARNING("Frames were in different child lists???");
  return 0;
}


nsIFrame* nsLayoutUtils::GetLastSibling(nsIFrame* aFrame) {
  if (!aFrame) {
    return nullptr;
  }

  nsIFrame* next;
  while ((next = aFrame->GetNextSibling()) != nullptr) {
    aFrame = next;
  }
  return aFrame;
}


nsIView*
nsLayoutUtils::FindSiblingViewFor(nsIView* aParentView, nsIFrame* aFrame) {
  nsIFrame* parentViewFrame = aParentView->GetFrame();
  nsIContent* parentViewContent = parentViewFrame ? parentViewFrame->GetContent() : nullptr;
  for (nsIView* insertBefore = aParentView->GetFirstChild(); insertBefore;
       insertBefore = insertBefore->GetNextSibling()) {
    nsIFrame* f = insertBefore->GetFrame();
    if (!f) {
      
      for (nsIView* searchView = insertBefore->GetParent(); searchView;
           searchView = searchView->GetParent()) {
        f = searchView->GetFrame();
        if (f) {
          break;
        }
      }
      NS_ASSERTION(f, "Can't find a frame anywhere!");
    }
    if (!f || !aFrame->GetContent() || !f->GetContent() ||
        CompareTreePosition(aFrame->GetContent(), f->GetContent(), parentViewContent) > 0) {
      
      
      return insertBefore;
    }
  }
  return nullptr;
}


nsIScrollableFrame*
nsLayoutUtils::GetScrollableFrameFor(nsIFrame *aScrolledFrame)
{
  nsIFrame *frame = aScrolledFrame->GetParent();
  if (!frame) {
    return nullptr;
  }
  nsIScrollableFrame *sf = do_QueryFrame(frame);
  return sf;
}

nsIFrame*
nsLayoutUtils::GetActiveScrolledRootFor(nsIFrame* aFrame,
                                        nsIFrame* aStopAtAncestor)
{
  nsIFrame* f = aFrame;
  while (f != aStopAtAncestor) {
    if (IsPopup(f))
      break;
    nsIFrame* parent = GetCrossDocParentFrame(f);
    if (!parent)
      break;
    nsIScrollableFrame* sf = do_QueryFrame(parent);
    if (sf && sf->IsScrollingActive() && sf->GetScrolledFrame() == f)
      break;
    f = parent;
  }
  return f;
}

nsIFrame*
nsLayoutUtils::GetActiveScrolledRootFor(nsDisplayItem* aItem,
                                        nsDisplayListBuilder* aBuilder,
                                        bool* aShouldFixToViewport)
{
  nsIFrame* f = aItem->GetUnderlyingFrame();
  if (aShouldFixToViewport) {
    *aShouldFixToViewport = false;
  }
  if (!f) {
    return nullptr;
  }
  if (aItem->ShouldFixToViewport(aBuilder)) {
    if (aShouldFixToViewport) {
      *aShouldFixToViewport = true;
    }
    
    
    
    
    nsIFrame* viewportFrame =
      nsLayoutUtils::GetClosestFrameOfType(f, nsGkAtoms::viewportFrame);
    NS_ASSERTION(viewportFrame, "no viewport???");
    return nsLayoutUtils::GetActiveScrolledRootFor(viewportFrame, aBuilder->ReferenceFrame());
  } else {
    return nsLayoutUtils::GetActiveScrolledRootFor(f, aBuilder->ReferenceFrame());
  }
}

bool
nsLayoutUtils::IsScrolledByRootContentDocumentDisplayportScrolling(nsIFrame* aActiveScrolledRoot,
                                                                   nsDisplayListBuilder* aBuilder)
{
  nsPresContext* presContext = aActiveScrolledRoot->PresContext()->
          GetToplevelContentDocumentPresContext();
  if (!presContext)
    return false;

  nsIFrame* rootScrollFrame = presContext->GetPresShell()->GetRootScrollFrame();
  if (!rootScrollFrame || !nsLayoutUtils::GetDisplayPort(rootScrollFrame->GetContent(), nullptr))
    return false;
  return nsLayoutUtils::IsAncestorFrameCrossDoc(rootScrollFrame, aActiveScrolledRoot);
}


nsIScrollableFrame*
nsLayoutUtils::GetNearestScrollableFrameForDirection(nsIFrame* aFrame,
                                                     Direction aDirection)
{
  NS_ASSERTION(aFrame, "GetNearestScrollableFrameForDirection expects a non-null frame");
  for (nsIFrame* f = aFrame; f; f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    nsIScrollableFrame* scrollableFrame = do_QueryFrame(f);
    if (scrollableFrame) {
      nsPresContext::ScrollbarStyles ss = scrollableFrame->GetScrollbarStyles();
      PRUint32 scrollbarVisibility = scrollableFrame->GetScrollbarVisibility();
      nsRect scrollRange = scrollableFrame->GetScrollRange();
      
      
      if (aDirection == eVertical ?
          (ss.mVertical != NS_STYLE_OVERFLOW_HIDDEN &&
           ((scrollbarVisibility & nsIScrollableFrame::VERTICAL) ||
            scrollRange.height > 0)) :
          (ss.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN &&
           ((scrollbarVisibility & nsIScrollableFrame::HORIZONTAL) ||
            scrollRange.width > 0)))
        return scrollableFrame;
    }
  }
  return nullptr;
}


nsIScrollableFrame*
nsLayoutUtils::GetNearestScrollableFrame(nsIFrame* aFrame)
{
  NS_ASSERTION(aFrame, "GetNearestScrollableFrame expects a non-null frame");
  for (nsIFrame* f = aFrame; f; f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    nsIScrollableFrame* scrollableFrame = do_QueryFrame(f);
    if (scrollableFrame) {
      nsPresContext::ScrollbarStyles ss = scrollableFrame->GetScrollbarStyles();
      if (ss.mVertical != NS_STYLE_OVERFLOW_HIDDEN ||
          ss.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN)
        return scrollableFrame;
    }
  }
  return nullptr;
}


bool
nsLayoutUtils::HasPseudoStyle(nsIContent* aContent,
                              nsStyleContext* aStyleContext,
                              nsCSSPseudoElements::Type aPseudoElement,
                              nsPresContext* aPresContext)
{
  NS_PRECONDITION(aPresContext, "Must have a prescontext");

  nsRefPtr<nsStyleContext> pseudoContext;
  if (aContent) {
    pseudoContext = aPresContext->StyleSet()->
      ProbePseudoElementStyle(aContent->AsElement(), aPseudoElement,
                              aStyleContext);
  }
  return pseudoContext != nullptr;
}

nsPoint
nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(nsIDOMEvent* aDOMEvent, nsIFrame* aFrame)
{
  if (!aDOMEvent)
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  nsEvent *event = aDOMEvent->GetInternalNSEvent();
  if (!event)
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  return GetEventCoordinatesRelativeTo(event, aFrame);
}

nsPoint
nsLayoutUtils::GetEventCoordinatesRelativeTo(const nsEvent* aEvent, nsIFrame* aFrame)
{
  if (!aEvent || (aEvent->eventStructType != NS_MOUSE_EVENT &&
                  aEvent->eventStructType != NS_MOUSE_SCROLL_EVENT &&
                  aEvent->eventStructType != NS_DRAG_EVENT &&
                  aEvent->eventStructType != NS_SIMPLE_GESTURE_EVENT &&
                  aEvent->eventStructType != NS_GESTURENOTIFY_EVENT &&
                  aEvent->eventStructType != NS_MOZTOUCH_EVENT &&
                  aEvent->eventStructType != NS_TOUCH_EVENT &&
                  aEvent->eventStructType != NS_QUERY_CONTENT_EVENT))
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);

  const nsGUIEvent* GUIEvent = static_cast<const nsGUIEvent*>(aEvent);
  return GetEventCoordinatesRelativeTo(aEvent,
                                       GUIEvent->refPoint,
                                       aFrame);
}

nsPoint
nsLayoutUtils::GetEventCoordinatesRelativeTo(const nsEvent* aEvent,
                                             const nsIntPoint aPoint,
                                             nsIFrame* aFrame)
{
  if (!aFrame) {
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }

  const nsGUIEvent* GUIEvent = static_cast<const nsGUIEvent*>(aEvent);
  nsIWidget* widget = GUIEvent->widget;
  if (!widget) {
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }

  nsIView* view = aFrame->GetView();
  if (view) {
    nsIWidget* frameWidget = view->GetWidget();
    if (frameWidget && frameWidget == GUIEvent->widget) {
      
      
      
      nsPresContext* presContext = aFrame->PresContext();
      nsPoint pt(presContext->DevPixelsToAppUnits(aPoint.x),
                 presContext->DevPixelsToAppUnits(aPoint.y));
      return pt - view->ViewToWidgetOffset();
    }
  }

  



  nsIFrame* rootFrame = aFrame;
  bool transformFound = false;
  for (nsIFrame* f = aFrame; f; f = GetCrossDocParentFrame(f)) {
    if (f->IsTransformed()) {
      transformFound = true;
    }

    rootFrame = f;
  }

  nsIView* rootView = rootFrame->GetView();
  if (!rootView) {
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }

  nsPoint widgetToView = TranslateWidgetToView(rootFrame->PresContext(),
                               widget, aPoint, rootView);

  if (widgetToView == nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE)) {
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }

  
  
  PRInt32 rootAPD = rootFrame->PresContext()->AppUnitsPerDevPixel();
  PRInt32 localAPD = aFrame->PresContext()->AppUnitsPerDevPixel();
  widgetToView = widgetToView.ConvertAppUnits(rootAPD, localAPD);

  


  if (transformFound) {
    return TransformRootPointToFrame(aFrame, widgetToView);
  }

  


  nsPoint offset = aFrame->GetOffsetToCrossDoc(rootFrame);
  return widgetToView - offset;
}

nsIFrame*
nsLayoutUtils::GetPopupFrameForEventCoordinates(nsPresContext* aPresContext,
                                                const nsEvent* aEvent)
{
#ifdef MOZ_XUL
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (!pm) {
    return nullptr;
  }
  nsTArray<nsIFrame*> popups = pm->GetVisiblePopups();
  PRUint32 i;
  
  for (i = 0; i < popups.Length(); i++) {
    nsIFrame* popup = popups[i];
    if (popup->PresContext()->GetRootPresContext() == aPresContext &&
        popup->GetScrollableOverflowRect().Contains(
          GetEventCoordinatesRelativeTo(aEvent, popup))) {
      return popup;
    }
  }
#endif
  return nullptr;
}

gfx3DMatrix
nsLayoutUtils::ChangeMatrixBasis(const gfxPoint3D &aOrigin,
                                 const gfx3DMatrix &aMatrix)
{
  gfx3DMatrix result = aMatrix;

  
  result.Translate(-aOrigin);

  
  result.TranslatePost(aOrigin);

  return result; 
}






static void ConstrainToCoordValues(gfxFloat &aVal)
{
  if (aVal <= nscoord_MIN)
    aVal = nscoord_MIN;
  else if (aVal >= nscoord_MAX)
    aVal = nscoord_MAX;
}

nsRect
nsLayoutUtils::RoundGfxRectToAppRect(const gfxRect &aRect, float aFactor)
{
  
  gfxRect scaledRect = aRect;
  scaledRect.ScaleRoundOut(aFactor);

  
  ConstrainToCoordValues(scaledRect.x);
  ConstrainToCoordValues(scaledRect.y);
  ConstrainToCoordValues(scaledRect.width);
  ConstrainToCoordValues(scaledRect.height);

  
  return nsRect(nscoord(scaledRect.X()), nscoord(scaledRect.Y()),
                nscoord(scaledRect.Width()), nscoord(scaledRect.Height()));
}


nsRegion
nsLayoutUtils::RoundedRectIntersectRect(const nsRect& aRoundedRect,
                                        const nscoord aRadii[8],
                                        const nsRect& aContainedRect)
{
  
  
  nsRect rectFullHeight = aRoundedRect;
  nscoord xDiff = NS_MAX(aRadii[NS_CORNER_TOP_LEFT_X], aRadii[NS_CORNER_BOTTOM_LEFT_X]);
  rectFullHeight.x += xDiff;
  rectFullHeight.width -= NS_MAX(aRadii[NS_CORNER_TOP_RIGHT_X],
                                 aRadii[NS_CORNER_BOTTOM_RIGHT_X]) + xDiff;
  nsRect r1;
  r1.IntersectRect(rectFullHeight, aContainedRect);

  nsRect rectFullWidth = aRoundedRect;
  nscoord yDiff = NS_MAX(aRadii[NS_CORNER_TOP_LEFT_Y], aRadii[NS_CORNER_TOP_RIGHT_Y]);
  rectFullWidth.y += yDiff;
  rectFullWidth.height -= NS_MAX(aRadii[NS_CORNER_BOTTOM_LEFT_Y],
                                 aRadii[NS_CORNER_BOTTOM_RIGHT_Y]) + yDiff;
  nsRect r2;
  r2.IntersectRect(rectFullWidth, aContainedRect);

  nsRegion result;
  result.Or(r1, r2);
  return result;
}

nsRect
nsLayoutUtils::MatrixTransformRectOut(const nsRect &aBounds,
                                      const gfx3DMatrix &aMatrix, float aFactor)
{
  nsRect outside = aBounds;
  outside.ScaleRoundOut(1/aFactor);
  gfxRect image = aMatrix.TransformBounds(gfxRect(outside.x,
                                                  outside.y,
                                                  outside.width,
                                                  outside.height));
  return RoundGfxRectToAppRect(image, aFactor);
}

nsRect
nsLayoutUtils::MatrixTransformRect(const nsRect &aBounds,
                                   const gfx3DMatrix &aMatrix, float aFactor)
{
  gfxRect image = aMatrix.TransformBounds(gfxRect(NSAppUnitsToDoublePixels(aBounds.x, aFactor),
                                                  NSAppUnitsToDoublePixels(aBounds.y, aFactor),
                                                  NSAppUnitsToDoublePixels(aBounds.width, aFactor),
                                                  NSAppUnitsToDoublePixels(aBounds.height, aFactor)));

  return RoundGfxRectToAppRect(image, aFactor);
}

nsPoint
nsLayoutUtils::MatrixTransformPoint(const nsPoint &aPoint,
                                    const gfx3DMatrix &aMatrix, float aFactor)
{
  gfxPoint image = aMatrix.Transform(gfxPoint(NSAppUnitsToFloatPixels(aPoint.x, aFactor),
                                              NSAppUnitsToFloatPixels(aPoint.y, aFactor)));
  return nsPoint(NSFloatPixelsToAppUnits(float(image.x), aFactor),
                 NSFloatPixelsToAppUnits(float(image.y), aFactor));
}

gfx3DMatrix
nsLayoutUtils::GetTransformToAncestor(nsIFrame *aFrame, nsIFrame *aAncestor)
{
  nsIFrame* parent;
  gfx3DMatrix ctm = aFrame->GetTransformMatrix(aAncestor, &parent);
  while (parent && parent != aAncestor) {
    if (!parent->Preserves3DChildren()) {
      ctm.ProjectTo2D();
    }
    ctm = ctm * parent->GetTransformMatrix(aAncestor, &parent);
  }
  return ctm;
}

static gfxPoint
TransformGfxPointFromAncestor(nsIFrame *aFrame,
                              const gfxPoint &aPoint,
                              nsIFrame *aAncestor)
{
  gfx3DMatrix ctm = nsLayoutUtils::GetTransformToAncestor(aFrame, aAncestor);
  return ctm.Inverse().ProjectPoint(aPoint);
}

static gfxRect
TransformGfxRectFromAncestor(nsIFrame *aFrame,
                             const gfxRect &aRect,
                             nsIFrame *aAncestor)
{
  gfx3DMatrix ctm = nsLayoutUtils::GetTransformToAncestor(aFrame, aAncestor);
  return ctm.Inverse().ProjectRectBounds(aRect);
}

static gfxRect
TransformGfxRectToAncestor(nsIFrame *aFrame,
                           const gfxRect &aRect,
                           nsIFrame *aAncestor)
{
  gfx3DMatrix ctm = nsLayoutUtils::GetTransformToAncestor(aFrame, aAncestor);
  return ctm.TransformBounds(aRect);
}

nsPoint
nsLayoutUtils::TransformRootPointToFrame(nsIFrame *aFrame,
                                         const nsPoint &aPoint)
{
    float factor = aFrame->PresContext()->AppUnitsPerDevPixel();
    gfxPoint result(NSAppUnitsToFloatPixels(aPoint.x, factor),
                    NSAppUnitsToFloatPixels(aPoint.y, factor));
    
    result = TransformGfxPointFromAncestor(aFrame, result, nullptr);
   
    return nsPoint(NSFloatPixelsToAppUnits(float(result.x), factor),
                   NSFloatPixelsToAppUnits(float(result.y), factor));
}

nsRect 
nsLayoutUtils::TransformAncestorRectToFrame(nsIFrame* aFrame,
                                            const nsRect &aRect,
                                            nsIFrame* aAncestor)
{
    float factor = aFrame->PresContext()->AppUnitsPerDevPixel();
    gfxRect result(NSAppUnitsToFloatPixels(aRect.x, factor),
                   NSAppUnitsToFloatPixels(aRect.y, factor),
                   NSAppUnitsToFloatPixels(aRect.width, factor),
                   NSAppUnitsToFloatPixels(aRect.height, factor));

    result = TransformGfxRectFromAncestor(aFrame, result, aAncestor);

    return nsRect(NSFloatPixelsToAppUnits(float(result.x), factor),
                  NSFloatPixelsToAppUnits(float(result.y), factor),
                  NSFloatPixelsToAppUnits(float(result.width), factor),
                  NSFloatPixelsToAppUnits(float(result.height), factor));
}

nsRect
nsLayoutUtils::TransformFrameRectToAncestor(nsIFrame* aFrame,
                                            const nsRect& aRect,
                                            nsIFrame* aAncestor)
{
  float factor = aFrame->PresContext()->AppUnitsPerDevPixel();
  gfxRect result(NSAppUnitsToFloatPixels(aRect.x, factor),
                 NSAppUnitsToFloatPixels(aRect.y, factor),
                 NSAppUnitsToFloatPixels(aRect.width, factor),
                 NSAppUnitsToFloatPixels(aRect.height, factor));

  result = TransformGfxRectToAncestor(aFrame, result, aAncestor);

  return nsRect(NSFloatPixelsToAppUnits(float(result.x), factor),
                NSFloatPixelsToAppUnits(float(result.y), factor),
                NSFloatPixelsToAppUnits(float(result.width), factor),
                NSFloatPixelsToAppUnits(float(result.height), factor));
}

static nsIntPoint GetWidgetOffset(nsIWidget* aWidget, nsIWidget*& aRootWidget) {
  nsIntPoint offset(0, 0);
  nsIWidget* parent = aWidget->GetParent();
  while (parent) {
    nsIntRect bounds;
    aWidget->GetBounds(bounds);
    offset += bounds.TopLeft();
    aWidget = parent;
    parent = aWidget->GetParent();
  }
  aRootWidget = aWidget;
  return offset;
}

nsPoint
nsLayoutUtils::TranslateWidgetToView(nsPresContext* aPresContext,
                                     nsIWidget* aWidget, nsIntPoint aPt,
                                     nsIView* aView)
{
  nsPoint viewOffset;
  nsIWidget* viewWidget = aView->GetNearestWidget(&viewOffset);
  if (!viewWidget) {
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }

  nsIWidget* fromRoot;
  nsIntPoint fromOffset = GetWidgetOffset(aWidget, fromRoot);
  nsIWidget* toRoot;
  nsIntPoint toOffset = GetWidgetOffset(viewWidget, toRoot);

  nsIntPoint widgetPoint;
  if (fromRoot == toRoot) {
    widgetPoint = aPt + fromOffset - toOffset;
  } else {
    nsIntPoint screenPoint = aWidget->WidgetToScreenOffset();
    widgetPoint = aPt + screenPoint - viewWidget->WidgetToScreenOffset();
  }

  nsPoint widgetAppUnits(aPresContext->DevPixelsToAppUnits(widgetPoint.x),
                         aPresContext->DevPixelsToAppUnits(widgetPoint.y));
  return widgetAppUnits - viewOffset;
}



PRUint8
nsLayoutUtils::CombineBreakType(PRUint8 aOrigBreakType,
                                PRUint8 aNewBreakType)
{
  PRUint8 breakType = aOrigBreakType;
  switch(breakType) {
  case NS_STYLE_CLEAR_LEFT:
    if ((NS_STYLE_CLEAR_RIGHT          == aNewBreakType) ||
        (NS_STYLE_CLEAR_LEFT_AND_RIGHT == aNewBreakType)) {
      breakType = NS_STYLE_CLEAR_LEFT_AND_RIGHT;
    }
    break;
  case NS_STYLE_CLEAR_RIGHT:
    if ((NS_STYLE_CLEAR_LEFT           == aNewBreakType) ||
        (NS_STYLE_CLEAR_LEFT_AND_RIGHT == aNewBreakType)) {
      breakType = NS_STYLE_CLEAR_LEFT_AND_RIGHT;
    }
    break;
  case NS_STYLE_CLEAR_NONE:
    if ((NS_STYLE_CLEAR_LEFT           == aNewBreakType) ||
        (NS_STYLE_CLEAR_RIGHT          == aNewBreakType) ||
        (NS_STYLE_CLEAR_LEFT_AND_RIGHT == aNewBreakType)) {
      breakType = aNewBreakType;
    }
  }
  return breakType;
}

#ifdef MOZ_DUMP_PAINTING
#include <stdio.h>

static bool gDumpEventList = false;
int gPaintCount = 0;
#endif

nsresult
nsLayoutUtils::GetRemoteContentIds(nsIFrame* aFrame,
                                   const nsRect& aTarget,
                                   nsTArray<ViewID> &aOutIDs,
                                   bool aIgnoreRootScrollFrame)
{
  nsDisplayListBuilder builder(aFrame, nsDisplayListBuilder::EVENT_DELIVERY,
                               false);
  nsDisplayList list;

  if (aIgnoreRootScrollFrame) {
    nsIFrame* rootScrollFrame =
      aFrame->PresContext()->PresShell()->GetRootScrollFrame();
    if (rootScrollFrame) {
      builder.SetIgnoreScrollFrame(rootScrollFrame);
    }
  }

  builder.EnterPresShell(aFrame, aTarget);

  nsresult rv =
    aFrame->BuildDisplayListForStackingContext(&builder, aTarget, &list);

  builder.LeavePresShell(aFrame, aTarget);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoTArray<nsIFrame*,8> outFrames;
  nsDisplayItem::HitTestState hitTestState(&aOutIDs);
  list.HitTest(&builder, aTarget, &hitTestState, &outFrames);
  list.DeleteAll();

  return NS_OK;
}

nsIFrame*
nsLayoutUtils::GetFrameForPoint(nsIFrame* aFrame, nsPoint aPt,
                                bool aShouldIgnoreSuppression,
                                bool aIgnoreRootScrollFrame)
{
  SAMPLE_LABEL("nsLayoutUtils", "GetFrameForPoint");
  nsresult rv;
  nsAutoTArray<nsIFrame*,8> outFrames;
  rv = GetFramesForArea(aFrame, nsRect(aPt, nsSize(1, 1)), outFrames,
                        aShouldIgnoreSuppression, aIgnoreRootScrollFrame);
  NS_ENSURE_SUCCESS(rv, nullptr);
  return outFrames.Length() ? outFrames.ElementAt(0) : nullptr;
}

nsresult
nsLayoutUtils::GetFramesForArea(nsIFrame* aFrame, const nsRect& aRect,
                                nsTArray<nsIFrame*> &aOutFrames,
                                bool aShouldIgnoreSuppression,
                                bool aIgnoreRootScrollFrame)
{
  SAMPLE_LABEL("nsLayoutUtils","GetFramesForArea");
  nsDisplayListBuilder builder(aFrame, nsDisplayListBuilder::EVENT_DELIVERY,
		                       false);
  nsDisplayList list;
  nsRect target(aRect);

  if (aShouldIgnoreSuppression) {
    builder.IgnorePaintSuppression();
  }

  if (aIgnoreRootScrollFrame) {
    nsIFrame* rootScrollFrame =
      aFrame->PresContext()->PresShell()->GetRootScrollFrame();
    if (rootScrollFrame) {
      builder.SetIgnoreScrollFrame(rootScrollFrame);
    }
  }

  builder.EnterPresShell(aFrame, target);

  nsresult rv =
    aFrame->BuildDisplayListForStackingContext(&builder, target, &list);

  builder.LeavePresShell(aFrame, target);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef MOZ_DUMP_PAINTING
  if (gDumpEventList) {
    fprintf(stdout, "Event handling --- (%d,%d):\n", aRect.x, aRect.y);
    nsFrame::PrintDisplayList(&builder, list);
  }
#endif

  nsDisplayItem::HitTestState hitTestState;
  list.HitTest(&builder, target, &hitTestState, &aOutFrames);
  list.DeleteAll();
  return NS_OK;
}











static void
PruneDisplayListForExtraPage(nsDisplayListBuilder* aBuilder,
        nsIFrame* aExtraPage, nscoord aY, nsDisplayList* aList)
{
  nsDisplayList newList;
  
  nsIFrame* mainPage = aBuilder->ReferenceFrame();

  while (true) {
    nsDisplayItem* i = aList->RemoveBottom();
    if (!i)
      break;
    nsDisplayList* subList = i->GetList();
    if (subList) {
      PruneDisplayListForExtraPage(aBuilder, aExtraPage, aY, subList);
      nsDisplayItem::Type type = i->GetType();
      if (type == nsDisplayItem::TYPE_CLIP ||
          type == nsDisplayItem::TYPE_CLIP_ROUNDED_RECT) {
        
        
        
        
        
        
        
        
        
        
        
        nsDisplayClip* clip = static_cast<nsDisplayClip*>(i);
        clip->SetClipRect(clip->GetClipRect() + nsPoint(0, aY) -
                aExtraPage->GetOffsetTo(mainPage));
      }
      newList.AppendToTop(i);
    } else {
      nsIFrame* f = i->GetUnderlyingFrame();
      if (f && nsLayoutUtils::IsProperAncestorFrameCrossDoc(mainPage, f)) {
        
        newList.AppendToTop(i);
      } else {
        
        
        i->~nsDisplayItem();
      }
    }
  }
  aList->AppendToTop(&newList);
}

static nsresult
BuildDisplayListForExtraPage(nsDisplayListBuilder* aBuilder,
        nsIFrame* aPage, nscoord aY, nsDisplayList* aList)
{
  nsDisplayList list;
  
  
  
  
  
  
  nsresult rv = aPage->BuildDisplayListForStackingContext(aBuilder, nsRect(), &list);
  if (NS_FAILED(rv))
    return rv;
  PruneDisplayListForExtraPage(aBuilder, aPage, aY, &list);
  aList->AppendToTop(&list);
  return NS_OK;
}

static nsIFrame*
GetNextPage(nsIFrame* aPageContentFrame)
{
  
  nsIFrame* pageFrame = aPageContentFrame->GetParent();
  NS_ASSERTION(pageFrame->GetType() == nsGkAtoms::pageFrame,
               "pageContentFrame has unexpected parent");
  nsIFrame* nextPageFrame = pageFrame->GetNextSibling();
  if (!nextPageFrame)
    return nullptr;
  NS_ASSERTION(nextPageFrame->GetType() == nsGkAtoms::pageFrame,
               "pageFrame's sibling is not a page frame...");
  nsIFrame* f = nextPageFrame->GetFirstPrincipalChild();
  NS_ASSERTION(f, "pageFrame has no page content frame!");
  NS_ASSERTION(f->GetType() == nsGkAtoms::pageContentFrame,
               "pageFrame's child is not page content!");
  return f;
}

nsresult
nsLayoutUtils::PaintFrame(nsRenderingContext* aRenderingContext, nsIFrame* aFrame,
                          const nsRegion& aDirtyRegion, nscolor aBackstop,
                          PRUint32 aFlags)
{
  SAMPLE_LABEL("nsLayoutUtils","PaintFrame");
  if (aFlags & PAINT_WIDGET_LAYERS) {
    nsIView* view = aFrame->GetView();
    if (!(view && view->GetWidget() && GetDisplayRootFrame(aFrame) == aFrame)) {
      aFlags &= ~PAINT_WIDGET_LAYERS;
      NS_ASSERTION(aRenderingContext, "need a rendering context");
    }
  }

  nsPresContext* presContext = aFrame->PresContext();
  nsIPresShell* presShell = presContext->PresShell();

  nsIFrame* rootScrollFrame = presShell->GetRootScrollFrame();
  bool usingDisplayPort = false;
  nsRect displayport;
  if (rootScrollFrame) {
    nsIContent* content = rootScrollFrame->GetContent();
    if (content) {
      usingDisplayPort = nsLayoutUtils::GetDisplayPort(content, &displayport);
    }
  }

  bool ignoreViewportScrolling = presShell->IgnoringViewportScrolling();
  nsRegion visibleRegion;
  if (aFlags & PAINT_WIDGET_LAYERS) {
    
    
    
    
    
    
    if (!usingDisplayPort) {
      visibleRegion = aFrame->GetVisualOverflowRectRelativeToSelf();
    } else {
      visibleRegion = displayport;
    }
  } else {
    visibleRegion = aDirtyRegion;
  }

  
  
  
  bool willFlushRetainedLayers = (aFlags & PAINT_HIDE_CARET) != 0;

  nsDisplayListBuilder builder(aFrame, nsDisplayListBuilder::PAINTING,
		                       !(aFlags & PAINT_HIDE_CARET));
  if (usingDisplayPort) {
    builder.SetDisplayPort(displayport);
  }

  nsDisplayList list;
  if (aFlags & PAINT_IN_TRANSFORM) {
    builder.SetInTransform(true);
  }
  if (aFlags & PAINT_SYNC_DECODE_IMAGES) {
    builder.SetSyncDecodeImages(true);
  }
  if (aFlags & (PAINT_WIDGET_LAYERS | PAINT_TO_WINDOW)) {
    builder.SetPaintingToWindow(true);
  }
  if (aFlags & PAINT_IGNORE_SUPPRESSION) {
    builder.IgnorePaintSuppression();
  }
  nsRect canvasArea(nsPoint(0, 0), aFrame->GetSize());

#ifdef DEBUG
  if (ignoreViewportScrolling) {
    nsIDocument* doc = aFrame->GetContent() ?
      aFrame->GetContent()->GetCurrentDoc() : nullptr;
    NS_ASSERTION(!aFrame->GetParent() ||
                 (doc && doc->IsBeingUsedAsImage()),
                 "Only expecting ignoreViewportScrolling for root frames and "
                 "for image documents.");
  }
#endif

  if (ignoreViewportScrolling && !aFrame->GetParent()) {
    nsIFrame* rootScrollFrame = presShell->GetRootScrollFrame();
    if (rootScrollFrame) {
      nsIScrollableFrame* rootScrollableFrame =
        presShell->GetRootScrollFrameAsScrollable();
      if (aFlags & PAINT_DOCUMENT_RELATIVE) {
        
        
        nsPoint pos = rootScrollableFrame->GetScrollPosition();
        visibleRegion.MoveBy(-pos);
        if (aRenderingContext) {
          aRenderingContext->Translate(pos);
        }
      }
      builder.SetIgnoreScrollFrame(rootScrollFrame);

      nsCanvasFrame* canvasFrame =
        do_QueryFrame(rootScrollableFrame->GetScrolledFrame());
      if (canvasFrame) {
        
        
        canvasArea.UnionRect(canvasArea,
          canvasFrame->CanvasArea() + builder.ToReferenceFrame(canvasFrame));
      }
    }
  }
  nsresult rv;

  nsRect dirtyRect = visibleRegion.GetBounds();
  builder.EnterPresShell(aFrame, dirtyRect);
  {
  SAMPLE_LABEL("nsLayoutUtils","PaintFrame::BuildDisplayList");
  rv = aFrame->BuildDisplayListForStackingContext(&builder, dirtyRect, &list);
  }
  const bool paintAllContinuations = aFlags & PAINT_ALL_CONTINUATIONS;
  NS_ASSERTION(!paintAllContinuations || !aFrame->GetPrevContinuation(),
               "If painting all continuations, the frame must be "
               "first-continuation");

  nsIAtom* frameType = aFrame->GetType();
  if (NS_SUCCEEDED(rv) && !paintAllContinuations &&
      frameType == nsGkAtoms::pageContentFrame) {
    NS_ASSERTION(!(aFlags & PAINT_WIDGET_LAYERS),
      "shouldn't be painting with widget layers for page content frames");
    
    
    
    
    
    
    
    nsIFrame* page = aFrame;
    nscoord y = aFrame->GetSize().height;
    while ((page = GetNextPage(page)) != nullptr) {
      SAMPLE_LABEL("nsLayoutUtils","PaintFrame::BuildDisplayListForExtraPage");
      rv = BuildDisplayListForExtraPage(&builder, page, y, &list);
      if (NS_FAILED(rv))
        break;
      y += page->GetSize().height;
    }
  }

  if (paintAllContinuations) {
    nsIFrame* currentFrame = aFrame;
    while (NS_SUCCEEDED(rv) &&
           (currentFrame = currentFrame->GetNextContinuation()) != nullptr) {
      SAMPLE_LABEL("nsLayoutUtils","PaintFrame::ContinuationsBuildDisplayList");
      nsRect frameDirty = dirtyRect - builder.ToReferenceFrame(currentFrame);
      rv = currentFrame->BuildDisplayListForStackingContext(&builder,
                                                            frameDirty, &list);
    }
  }

  
  
  if (frameType == nsGkAtoms::viewportFrame && 
      nsLayoutUtils::NeedsPrintPreviewBackground(presContext)) {
    nsRect bounds = nsRect(builder.ToReferenceFrame(aFrame),
                           aFrame->GetSize());
    rv = presShell->AddPrintPreviewBackgroundItem(builder, list, aFrame, bounds);
  } else if (frameType != nsGkAtoms::pageFrame) {
    
    
    
    
    

    
    
    
    canvasArea.IntersectRect(canvasArea, visibleRegion.GetBounds());
    rv = presShell->AddCanvasBackgroundColorItem(
           builder, list, aFrame, canvasArea, aBackstop);

    
    
    if ((aFlags & PAINT_WIDGET_LAYERS) && !willFlushRetainedLayers) {
      nsIView* view = aFrame->GetView();
      if (view) {
        nscolor backstop = presShell->ComputeBackstopColor(view);
        
        
        nscolor canvasColor = presShell->GetCanvasBackground();
        if (NS_ComposeColors(aBackstop, canvasColor) !=
            NS_ComposeColors(backstop, canvasColor)) {
          willFlushRetainedLayers = true;
        }
      }
    }
  }

  builder.LeavePresShell(aFrame, dirtyRect);
  NS_ENSURE_SUCCESS(rv, rv);

  if (builder.GetHadToIgnorePaintSuppression()) {
    willFlushRetainedLayers = true;
  }

#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPaintList || gfxUtils::sDumpPainting) {
    if (gfxUtils::sDumpPaintingToFile) {
      nsCString string("dump-");
      string.AppendInt(gPaintCount);
      string.Append(".html");
      gfxUtils::sDumpPaintFile = fopen(string.BeginReading(), "w");
    } else {
      gfxUtils::sDumpPaintFile = stdout;
    }
    fprintf(gfxUtils::sDumpPaintFile, "<html><head><script>var array = {}; function ViewImage(index) { window.location = array[index]; }</script></head><body>");
    fprintf(gfxUtils::sDumpPaintFile, "Painting --- before optimization (dirty %d,%d,%d,%d):\n",
            dirtyRect.x, dirtyRect.y, dirtyRect.width, dirtyRect.height);
    nsFrame::PrintDisplayList(&builder, list, gfxUtils::sDumpPaintFile);
    if (gfxUtils::sDumpPaintingToFile) {
      fprintf(gfxUtils::sDumpPaintFile, "<script>");
    }
  }
#endif

  list.ComputeVisibilityForRoot(&builder, &visibleRegion);

  PRUint32 flags = nsDisplayList::PAINT_DEFAULT;
  if (aFlags & PAINT_WIDGET_LAYERS) {
    flags |= nsDisplayList::PAINT_USE_WIDGET_LAYERS;
    if (willFlushRetainedLayers) {
      
      
      
      
      
      
      NS_WARNING("Flushing retained layers!");
      flags |= nsDisplayList::PAINT_FLUSH_LAYERS;
    } else if (!(aFlags & PAINT_DOCUMENT_RELATIVE)) {
      nsIWidget *widget = aFrame->GetNearestWidget();
      if (widget) {
        builder.SetFinalTransparentRegion(visibleRegion);
        
        
        widget->UpdateThemeGeometries(builder.GetThemeGeometries());
      }
    }
  }
  if (aFlags & PAINT_EXISTING_TRANSACTION) {
    flags |= nsDisplayList::PAINT_EXISTING_TRANSACTION;
  }

  list.PaintRoot(&builder, aRenderingContext, flags);

  
  
  if ((aFlags & PAINT_WIDGET_LAYERS) &&
      !willFlushRetainedLayers &&
      !(aFlags & PAINT_DOCUMENT_RELATIVE)) {
    nsIWidget *widget = aFrame->GetNearestWidget();
    if (widget) {
      nsRegion excludedRegion = builder.GetExcludedGlassRegion();
      excludedRegion.Sub(excludedRegion, visibleRegion);
      nsIntRegion windowRegion(excludedRegion.ToNearestPixels(presContext->AppUnitsPerDevPixel()));
      widget->UpdateOpaqueRegion(windowRegion);
    }
  }

#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPaintList || gfxUtils::sDumpPainting) {
    fprintf(gfxUtils::sDumpPaintFile, "</script>Painting --- after optimization:\n");
    nsFrame::PrintDisplayList(&builder, list, gfxUtils::sDumpPaintFile);

    fprintf(gfxUtils::sDumpPaintFile, "Painting --- retained layer tree:\n");
    nsIWidget* widget = aFrame->GetNearestWidget();
    if (widget) {
      nsRefPtr<LayerManager> layerManager = widget->GetLayerManager();
      if (layerManager) {
        FrameLayerBuilder::DumpRetainedLayerTree(layerManager, gfxUtils::sDumpPaintFile);
      }
    }
    fprintf(gfxUtils::sDumpPaintFile, "</body></html>");
    
    if (gfxUtils::sDumpPaintingToFile) {
      fclose(gfxUtils::sDumpPaintFile);
    }
    gfxUtils::sDumpPaintFile = NULL;
    gPaintCount++;
  }
#endif

  
  list.DeleteAll();
  return NS_OK;
}

PRInt32
nsLayoutUtils::GetZIndex(nsIFrame* aFrame) {
  if (!aFrame->IsPositioned())
    return 0;

  const nsStylePosition* position =
    aFrame->GetStylePosition();
  if (position->mZIndex.GetUnit() == eStyleUnit_Integer)
    return position->mZIndex.GetIntValue();

  
  return 0;
}













bool
nsLayoutUtils::BinarySearchForPosition(nsRenderingContext* aRendContext,
                        const PRUnichar* aText,
                        PRInt32    aBaseWidth,
                        PRInt32    aBaseInx,
                        PRInt32    aStartInx,
                        PRInt32    aEndInx,
                        PRInt32    aCursorPos,
                        PRInt32&   aIndex,
                        PRInt32&   aTextWidth)
{
  PRInt32 range = aEndInx - aStartInx;
  if ((range == 1) || (range == 2 && NS_IS_HIGH_SURROGATE(aText[aStartInx]))) {
    aIndex   = aStartInx + aBaseInx;
    aTextWidth = aRendContext->GetWidth(aText, aIndex);
    return true;
  }

  PRInt32 inx = aStartInx + (range / 2);

  
  if (NS_IS_HIGH_SURROGATE(aText[inx-1]))
    inx++;

  PRInt32 textWidth = aRendContext->GetWidth(aText, inx);

  PRInt32 fullWidth = aBaseWidth + textWidth;
  if (fullWidth == aCursorPos) {
    aTextWidth = textWidth;
    aIndex = inx;
    return true;
  } else if (aCursorPos < fullWidth) {
    aTextWidth = aBaseWidth;
    if (BinarySearchForPosition(aRendContext, aText, aBaseWidth, aBaseInx, aStartInx, inx, aCursorPos, aIndex, aTextWidth)) {
      return true;
    }
  } else {
    aTextWidth = fullWidth;
    if (BinarySearchForPosition(aRendContext, aText, aBaseWidth, aBaseInx, inx, aEndInx, aCursorPos, aIndex, aTextWidth)) {
      return true;
    }
  }
  return false;
}

static void
AddBoxesForFrame(nsIFrame* aFrame,
                 nsLayoutUtils::BoxCallback* aCallback)
{
  nsIAtom* pseudoType = aFrame->GetStyleContext()->GetPseudo();

  if (pseudoType == nsCSSAnonBoxes::tableOuter) {
    AddBoxesForFrame(aFrame->GetFirstPrincipalChild(), aCallback);
    nsIFrame* kid = aFrame->GetFirstChild(nsIFrame::kCaptionList);
    if (kid) {
      AddBoxesForFrame(kid, aCallback);
    }
  } else if (pseudoType == nsCSSAnonBoxes::mozAnonymousBlock ||
             pseudoType == nsCSSAnonBoxes::mozAnonymousPositionedBlock ||
             pseudoType == nsCSSAnonBoxes::mozMathMLAnonymousBlock ||
             pseudoType == nsCSSAnonBoxes::mozXULAnonymousBlock) {
    for (nsIFrame* kid = aFrame->GetFirstPrincipalChild(); kid; kid = kid->GetNextSibling()) {
      AddBoxesForFrame(kid, aCallback);
    }
  } else {
    aCallback->AddBox(aFrame);
  }
}

void
nsLayoutUtils::GetAllInFlowBoxes(nsIFrame* aFrame, BoxCallback* aCallback)
{
  while (aFrame) {
    AddBoxesForFrame(aFrame, aCallback);
    aFrame = nsLayoutUtils::GetNextContinuationOrSpecialSibling(aFrame);
  }
}

struct BoxToRect : public nsLayoutUtils::BoxCallback {
  typedef nsSize (*GetRectFromFrameFun)(nsIFrame*);

  nsIFrame* mRelativeTo;
  nsLayoutUtils::RectCallback* mCallback;
  PRUint32 mFlags;
  GetRectFromFrameFun mRectFromFrame;

  BoxToRect(nsIFrame* aRelativeTo, nsLayoutUtils::RectCallback* aCallback,
            PRUint32 aFlags, GetRectFromFrameFun aRectFromFrame)
    : mRelativeTo(aRelativeTo), mCallback(aCallback), mFlags(aFlags),
      mRectFromFrame(aRectFromFrame) {}

  virtual void AddBox(nsIFrame* aFrame) {
    nsRect r;
    nsIFrame* outer = nsSVGUtils::GetOuterSVGFrameAndCoveredRegion(aFrame, &r);
    if (!outer) {
      outer = aFrame;
      r = nsRect(nsPoint(0, 0), mRectFromFrame(aFrame));
    }
    if (mFlags & nsLayoutUtils::RECTS_ACCOUNT_FOR_TRANSFORMS) {
      r = nsLayoutUtils::TransformFrameRectToAncestor(outer, r, mRelativeTo);
    } else {
      r += outer->GetOffsetTo(mRelativeTo);
    }
    mCallback->AddRect(r);
  }
};

static nsSize
GetFrameBorderSize(nsIFrame* aFrame)
{
  return aFrame->GetSize();
}

void
nsLayoutUtils::GetAllInFlowRects(nsIFrame* aFrame, nsIFrame* aRelativeTo,
                                 RectCallback* aCallback, PRUint32 aFlags)
{
  BoxToRect converter(aRelativeTo, aCallback, aFlags, &GetFrameBorderSize);
  GetAllInFlowBoxes(aFrame, &converter);
}

static nsSize
GetFramePaddingSize(nsIFrame* aFrame)
{
  return aFrame->GetPaddingRect().Size();
}

void
nsLayoutUtils::GetAllInFlowPaddingRects(nsIFrame* aFrame, nsIFrame* aRelativeTo,
                                        RectCallback* aCallback, PRUint32 aFlags)
{
  BoxToRect converter(aRelativeTo, aCallback, aFlags, &GetFramePaddingSize);
  GetAllInFlowBoxes(aFrame, &converter);
}

nsLayoutUtils::RectAccumulator::RectAccumulator() : mSeenFirstRect(false) {}

void nsLayoutUtils::RectAccumulator::AddRect(const nsRect& aRect) {
  mResultRect.UnionRect(mResultRect, aRect);
  if (!mSeenFirstRect) {
    mSeenFirstRect = true;
    mFirstRect = aRect;
  }
}

nsLayoutUtils::RectListBuilder::RectListBuilder(nsClientRectList* aList)
  : mRectList(aList), mRV(NS_OK) {}

void nsLayoutUtils::RectListBuilder::AddRect(const nsRect& aRect) {
  nsRefPtr<nsClientRect> rect = new nsClientRect();

  rect->SetLayoutRect(aRect);
  mRectList->Append(rect);
}

nsIFrame* nsLayoutUtils::GetContainingBlockForClientRect(nsIFrame* aFrame)
{
  return aFrame->PresContext()->PresShell()->GetRootFrame();
}

nsRect
nsLayoutUtils::GetAllInFlowRectsUnion(nsIFrame* aFrame, nsIFrame* aRelativeTo,
                                      PRUint32 aFlags) {
  RectAccumulator accumulator;
  GetAllInFlowRects(aFrame, aRelativeTo, &accumulator, aFlags);
  return accumulator.mResultRect.IsEmpty() ? accumulator.mFirstRect
          : accumulator.mResultRect;
}

nsRect
nsLayoutUtils::GetAllInFlowPaddingRectsUnion(nsIFrame* aFrame,
                                             nsIFrame* aRelativeTo,
                                             PRUint32 aFlags)
{
  RectAccumulator accumulator;
  GetAllInFlowPaddingRects(aFrame, aRelativeTo, &accumulator, aFlags);
  return accumulator.mResultRect.IsEmpty() ? accumulator.mFirstRect
          : accumulator.mResultRect;
}

nsRect
nsLayoutUtils::GetTextShadowRectsUnion(const nsRect& aTextAndDecorationsRect,
                                       nsIFrame* aFrame,
                                       PRUint32 aFlags)
{
  const nsStyleText* textStyle = aFrame->GetStyleText();
  if (!textStyle->mTextShadow)
    return aTextAndDecorationsRect;

  nsRect resultRect = aTextAndDecorationsRect;
  PRInt32 A2D = aFrame->PresContext()->AppUnitsPerDevPixel();
  for (PRUint32 i = 0; i < textStyle->mTextShadow->Length(); ++i) {
    nsCSSShadowItem* shadow = textStyle->mTextShadow->ShadowAt(i);
    nsMargin blur = nsContextBoxBlur::GetBlurRadiusMargin(shadow->mRadius, A2D);
    if ((aFlags & EXCLUDE_BLUR_SHADOWS) && blur != nsMargin(0, 0, 0, 0))
      continue;

    nsRect tmpRect(aTextAndDecorationsRect);

    tmpRect.MoveBy(nsPoint(shadow->mXOffset, shadow->mYOffset));
    tmpRect.Inflate(blur);

    resultRect.UnionRect(resultRect, tmpRect);
  }
  return resultRect;
}

nsresult
nsLayoutUtils::GetFontMetricsForFrame(const nsIFrame* aFrame,
                                      nsFontMetrics** aFontMetrics,
                                      float aInflation)
{
  return nsLayoutUtils::GetFontMetricsForStyleContext(aFrame->GetStyleContext(),
                                                      aFontMetrics,
                                                      aInflation);
}

nsresult
nsLayoutUtils::GetFontMetricsForStyleContext(nsStyleContext* aStyleContext,
                                             nsFontMetrics** aFontMetrics,
                                             float aInflation)
{
  
  gfxUserFontSet* fs = aStyleContext->PresContext()->GetUserFontSet();

  nsFont font = aStyleContext->GetStyleFont()->mFont;
  
  
  
  if (aInflation != 1.0f) {
    font.size = NSToCoordRound(font.size * aInflation);
  }
  return aStyleContext->PresContext()->DeviceContext()->GetMetricsFor(
                  font, aStyleContext->GetStyleFont()->mLanguage,
                  fs, *aFontMetrics);
}

nsIFrame*
nsLayoutUtils::FindChildContainingDescendant(nsIFrame* aParent, nsIFrame* aDescendantFrame)
{
  nsIFrame* result = aDescendantFrame;

  while (result) {
    nsIFrame* parent = result->GetParent();
    if (parent == aParent) {
      break;
    }

    
    result = parent;
  }

  return result;
}

nsBlockFrame*
nsLayoutUtils::GetAsBlock(nsIFrame* aFrame)
{
  nsBlockFrame* block = do_QueryFrame(aFrame);
  return block;
}

nsBlockFrame*
nsLayoutUtils::FindNearestBlockAncestor(nsIFrame* aFrame)
{
  nsIFrame* nextAncestor;
  for (nextAncestor = aFrame->GetParent(); nextAncestor;
       nextAncestor = nextAncestor->GetParent()) {
    nsBlockFrame* block = GetAsBlock(nextAncestor);
    if (block)
      return block;
  }
  return nullptr;
}

nsIFrame*
nsLayoutUtils::GetNonGeneratedAncestor(nsIFrame* aFrame)
{
  if (!(aFrame->GetStateBits() & NS_FRAME_GENERATED_CONTENT))
    return aFrame;

  nsIFrame* f = aFrame;
  do {
    f = GetParentOrPlaceholderFor(f);
  } while (f->GetStateBits() & NS_FRAME_GENERATED_CONTENT);
  return f;
}

nsIFrame*
nsLayoutUtils::GetParentOrPlaceholderFor(nsIFrame* aFrame)
{
  if ((aFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)
      && !aFrame->GetPrevInFlow()) {
    return aFrame->PresContext()->PresShell()->FrameManager()->
      GetPlaceholderFrameFor(aFrame);
  }
  return aFrame->GetParent();
}

nsIFrame*
nsLayoutUtils::GetParentOrPlaceholderForCrossDoc(nsIFrame* aFrame)
{
  nsIFrame* f = GetParentOrPlaceholderFor(aFrame);
  if (f)
    return f;
  return GetCrossDocParentFrame(aFrame);
}

nsIFrame*
nsLayoutUtils::GetNextContinuationOrSpecialSibling(nsIFrame *aFrame)
{
  nsIFrame *result = aFrame->GetNextContinuation();
  if (result)
    return result;

  if ((aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) != 0) {
    
    
    aFrame = aFrame->GetFirstContinuation();

    void* value = aFrame->Properties().Get(nsIFrame::IBSplitSpecialSibling());
    return static_cast<nsIFrame*>(value);
  }

  return nullptr;
}

nsIFrame*
nsLayoutUtils::GetFirstContinuationOrSpecialSibling(nsIFrame *aFrame)
{
  nsIFrame *result = aFrame->GetFirstContinuation();
  if (result->GetStateBits() & NS_FRAME_IS_SPECIAL) {
    while (true) {
      nsIFrame *f = static_cast<nsIFrame*>
        (result->Properties().Get(nsIFrame::IBSplitSpecialPrevSibling()));
      if (!f)
        break;
      result = f;
    }
  }

  return result;
}

bool
nsLayoutUtils::IsViewportScrollbarFrame(nsIFrame* aFrame)
{
  if (!aFrame)
    return false;

  nsIFrame* rootScrollFrame =
    aFrame->PresContext()->PresShell()->GetRootScrollFrame();
  if (!rootScrollFrame)
    return false;

  nsIScrollableFrame* rootScrollableFrame = do_QueryFrame(rootScrollFrame);
  NS_ASSERTION(rootScrollableFrame, "The root scorollable frame is null");

  if (!IsProperAncestorFrame(rootScrollFrame, aFrame))
    return false;

  nsIFrame* rootScrolledFrame = rootScrollableFrame->GetScrolledFrame();
  return !(rootScrolledFrame == aFrame ||
           IsProperAncestorFrame(rootScrolledFrame, aFrame));
}

static nscoord AddPercents(nsLayoutUtils::IntrinsicWidthType aType,
                           nscoord aCurrent, float aPercent)
{
  nscoord result = aCurrent;
  if (aPercent > 0.0f && aType == nsLayoutUtils::PREF_WIDTH) {
    
    
    if (aPercent >= 1.0f)
      result = nscoord_MAX;
    else
      result = NSToCoordRound(float(result) / (1.0f - aPercent));
  }
  return result;
}



static bool GetAbsoluteCoord(const nsStyleCoord& aStyle, nscoord& aResult)
{
  if (aStyle.IsCalcUnit()) {
    if (aStyle.CalcHasPercent()) {
      return false;
    }
    
    aResult = nsRuleNode::ComputeComputedCalc(aStyle, 0);
    if (aResult < 0)
      aResult = 0;
    return true;
  }

  if (eStyleUnit_Coord != aStyle.GetUnit())
    return false;

  aResult = aStyle.GetCoordValue();
  NS_ASSERTION(aResult >= 0, "negative widths not allowed");
  return true;
}

static bool
GetPercentHeight(const nsStyleCoord& aStyle,
                 nsIFrame* aFrame,
                 nscoord& aResult)
{
  if (eStyleUnit_Percent != aStyle.GetUnit())
    return false;

  nsIFrame *f = aFrame->GetContainingBlock();
  if (!f) {
    NS_NOTREACHED("top of frame tree not a containing block");
    return false;
  }

  const nsStylePosition *pos = f->GetStylePosition();
  nscoord h;
  if (!GetAbsoluteCoord(pos->mHeight, h) &&
      !GetPercentHeight(pos->mHeight, f, h)) {
    NS_ASSERTION(pos->mHeight.GetUnit() == eStyleUnit_Auto ||
                 pos->mHeight.HasPercent(),
                 "unknown height unit");
    nsIAtom* fType = f->GetType();
    if (fType != nsGkAtoms::viewportFrame && fType != nsGkAtoms::canvasFrame &&
        fType != nsGkAtoms::pageContentFrame) {
      
      
      
      
      
      return false;
    }

    NS_ASSERTION(pos->mHeight.GetUnit() == eStyleUnit_Auto,
                 "Unexpected height unit for viewport or canvas or page-content");
    
    
    h = f->GetSize().height;
    if (h == NS_UNCONSTRAINEDSIZE) {
      
      return false;
    }
  }

  nscoord maxh;
  if (GetAbsoluteCoord(pos->mMaxHeight, maxh) ||
      GetPercentHeight(pos->mMaxHeight, f, maxh)) {
    if (maxh < h)
      h = maxh;
  } else {
    NS_ASSERTION(pos->mMaxHeight.GetUnit() == eStyleUnit_None ||
                 pos->mMaxHeight.HasPercent(),
                 "unknown max-height unit");
  }

  nscoord minh;
  if (GetAbsoluteCoord(pos->mMinHeight, minh) ||
      GetPercentHeight(pos->mMinHeight, f, minh)) {
    if (minh > h)
      h = minh;
  } else {
    NS_ASSERTION(pos->mMinHeight.HasPercent(),
                 "unknown min-height unit");
  }

  aResult = NSToCoordRound(aStyle.GetPercentValue() * h);
  return true;
}





enum eWidthProperty { PROP_WIDTH, PROP_MAX_WIDTH, PROP_MIN_WIDTH };
static bool
GetIntrinsicCoord(const nsStyleCoord& aStyle,
                  nsRenderingContext* aRenderingContext,
                  nsIFrame* aFrame,
                  eWidthProperty aProperty,
                  nscoord& aResult)
{
  NS_PRECONDITION(aProperty == PROP_WIDTH || aProperty == PROP_MAX_WIDTH ||
                  aProperty == PROP_MIN_WIDTH, "unexpected property");
  if (aStyle.GetUnit() != eStyleUnit_Enumerated)
    return false;
  PRInt32 val = aStyle.GetIntValue();
  NS_ASSERTION(val == NS_STYLE_WIDTH_MAX_CONTENT ||
               val == NS_STYLE_WIDTH_MIN_CONTENT ||
               val == NS_STYLE_WIDTH_FIT_CONTENT ||
               val == NS_STYLE_WIDTH_AVAILABLE,
               "unexpected enumerated value for width property");
  if (val == NS_STYLE_WIDTH_AVAILABLE)
    return false;
  if (val == NS_STYLE_WIDTH_FIT_CONTENT) {
    if (aProperty == PROP_WIDTH)
      return false; 
    if (aProperty == PROP_MAX_WIDTH)
      
      val = NS_STYLE_WIDTH_MAX_CONTENT;
    else
      
      val = NS_STYLE_WIDTH_MIN_CONTENT;
  }

  NS_ASSERTION(val == NS_STYLE_WIDTH_MAX_CONTENT ||
               val == NS_STYLE_WIDTH_MIN_CONTENT,
               "should have reduced everything remaining to one of these");

  
  
  AutoMaybeDisableFontInflation an(aFrame);

  if (val == NS_STYLE_WIDTH_MAX_CONTENT)
    aResult = aFrame->GetPrefWidth(aRenderingContext);
  else
    aResult = aFrame->GetMinWidth(aRenderingContext);
  return true;
}

#undef  DEBUG_INTRINSIC_WIDTH

#ifdef DEBUG_INTRINSIC_WIDTH
static PRInt32 gNoiseIndent = 0;
#endif

 nscoord
nsLayoutUtils::IntrinsicForContainer(nsRenderingContext *aRenderingContext,
                                     nsIFrame *aFrame,
                                     IntrinsicWidthType aType)
{
  NS_PRECONDITION(aFrame, "null frame");
  NS_PRECONDITION(aType == MIN_WIDTH || aType == PREF_WIDTH, "bad type");

#ifdef DEBUG_INTRINSIC_WIDTH
  nsFrame::IndentBy(stdout, gNoiseIndent);
  static_cast<nsFrame*>(aFrame)->ListTag(stdout);
  printf(" %s intrinsic width for container:\n",
         aType == MIN_WIDTH ? "min" : "pref");
#endif

  
  
  AutoMaybeDisableFontInflation an(aFrame);

  nsIFrame::IntrinsicWidthOffsetData offsets =
    aFrame->IntrinsicWidthOffsets(aRenderingContext);

  const nsStylePosition *stylePos = aFrame->GetStylePosition();
  PRUint8 boxSizing = stylePos->mBoxSizing;
  const nsStyleCoord &styleWidth = stylePos->mWidth;
  const nsStyleCoord &styleMinWidth = stylePos->mMinWidth;
  const nsStyleCoord &styleMaxWidth = stylePos->mMaxWidth;

  
  
  
  
  
  
  
  
  
  
  nscoord result = 0, min = 0;

  nscoord maxw;
  bool haveFixedMaxWidth = GetAbsoluteCoord(styleMaxWidth, maxw);
  nscoord minw;
  bool haveFixedMinWidth = GetAbsoluteCoord(styleMinWidth, minw);

  
  
  
  if (styleWidth.GetUnit() == eStyleUnit_Enumerated &&
      (styleWidth.GetIntValue() == NS_STYLE_WIDTH_MAX_CONTENT ||
       styleWidth.GetIntValue() == NS_STYLE_WIDTH_MIN_CONTENT)) {
    
    
    
    
    boxSizing = NS_STYLE_BOX_SIZING_CONTENT;
  } else if (styleWidth.GetUnit() != eStyleUnit_Coord &&
             !(haveFixedMinWidth && haveFixedMaxWidth && maxw <= minw)) {
#ifdef DEBUG_INTRINSIC_WIDTH
    ++gNoiseIndent;
#endif
    if (aType == MIN_WIDTH)
      result = aFrame->GetMinWidth(aRenderingContext);
    else
      result = aFrame->GetPrefWidth(aRenderingContext);
#ifdef DEBUG_INTRINSIC_WIDTH
    --gNoiseIndent;
    nsFrame::IndentBy(stdout, gNoiseIndent);
    static_cast<nsFrame*>(aFrame)->ListTag(stdout);
    printf(" %s intrinsic width from frame is %d.\n",
           aType == MIN_WIDTH ? "min" : "pref", result);
#endif

    
    
    const nsStyleCoord &styleHeight = stylePos->mHeight;
    const nsStyleCoord &styleMinHeight = stylePos->mMinHeight;
    const nsStyleCoord &styleMaxHeight = stylePos->mMaxHeight;
    if (styleHeight.GetUnit() != eStyleUnit_Auto ||
        !(styleMinHeight.GetUnit() == eStyleUnit_Coord &&
          styleMinHeight.GetCoordValue() == 0) ||
        styleMaxHeight.GetUnit() != eStyleUnit_None) {

      nsSize ratio = aFrame->GetIntrinsicRatio();

      if (ratio.height != 0) {

        nscoord h;
        if (GetAbsoluteCoord(styleHeight, h) ||
            GetPercentHeight(styleHeight, aFrame, h)) {
          result =
            NSToCoordRound(h * (float(ratio.width) / float(ratio.height)));
        }

        if (GetAbsoluteCoord(styleMaxHeight, h) ||
            GetPercentHeight(styleMaxHeight, aFrame, h)) {
          h = NSToCoordRound(h * (float(ratio.width) / float(ratio.height)));
          if (h < result)
            result = h;
        }

        if (GetAbsoluteCoord(styleMinHeight, h) ||
            GetPercentHeight(styleMinHeight, aFrame, h)) {
          h = NSToCoordRound(h * (float(ratio.width) / float(ratio.height)));
          if (h > result)
            result = h;
        }
      }
    }
  }

  if (aFrame->GetType() == nsGkAtoms::tableFrame) {
    
    
    min = aFrame->GetMinWidth(aRenderingContext);
  }

  
  
  
  
  
  
  
  nscoord coordOutsideWidth = offsets.hPadding;
  float pctOutsideWidth = offsets.hPctPadding;

  float pctTotal = 0.0f;

  if (boxSizing == NS_STYLE_BOX_SIZING_PADDING) {
    min += coordOutsideWidth;
    result = NSCoordSaturatingAdd(result, coordOutsideWidth);
    pctTotal += pctOutsideWidth;

    coordOutsideWidth = 0;
    pctOutsideWidth = 0.0f;
  }

  coordOutsideWidth += offsets.hBorder;

  if (boxSizing == NS_STYLE_BOX_SIZING_BORDER) {
    min += coordOutsideWidth;
    result = NSCoordSaturatingAdd(result, coordOutsideWidth);
    pctTotal += pctOutsideWidth;

    coordOutsideWidth = 0;
    pctOutsideWidth = 0.0f;
  }

  coordOutsideWidth += offsets.hMargin;
  pctOutsideWidth += offsets.hPctMargin;

  min += coordOutsideWidth;
  result = NSCoordSaturatingAdd(result, coordOutsideWidth);
  pctTotal += pctOutsideWidth;

  nscoord w;
  if (GetAbsoluteCoord(styleWidth, w) ||
      GetIntrinsicCoord(styleWidth, aRenderingContext, aFrame,
                        PROP_WIDTH, w)) {
    result = AddPercents(aType, w + coordOutsideWidth, pctOutsideWidth);
  }
  else if (aType == MIN_WIDTH &&
           
           
           
           styleWidth.IsCoordPercentCalcUnit() &&
           aFrame->IsFrameOfType(nsIFrame::eReplaced)) {
    
    result = 0; 
  }
  else {
    
    
    
    
    
    result = AddPercents(aType, result, pctTotal);
  }

  if (haveFixedMaxWidth ||
      GetIntrinsicCoord(styleMaxWidth, aRenderingContext, aFrame,
                        PROP_MAX_WIDTH, maxw)) {
    maxw = AddPercents(aType, maxw + coordOutsideWidth, pctOutsideWidth);
    if (result > maxw)
      result = maxw;
  }

  if (haveFixedMinWidth ||
      GetIntrinsicCoord(styleMinWidth, aRenderingContext, aFrame,
                        PROP_MIN_WIDTH, minw)) {
    minw = AddPercents(aType, minw + coordOutsideWidth, pctOutsideWidth);
    if (result < minw)
      result = minw;
  }

  min = AddPercents(aType, min, pctTotal);
  if (result < min)
    result = min;

  const nsStyleDisplay *disp = aFrame->GetStyleDisplay();
  if (aFrame->IsThemed(disp)) {
    nsIntSize size(0, 0);
    bool canOverride = true;
    nsPresContext *presContext = aFrame->PresContext();
    presContext->GetTheme()->
      GetMinimumWidgetSize(aRenderingContext, aFrame, disp->mAppearance,
                           &size, &canOverride);

    nscoord themeWidth = presContext->DevPixelsToAppUnits(size.width);

    
    themeWidth += offsets.hMargin;
    themeWidth = AddPercents(aType, themeWidth, offsets.hPctMargin);

    if (themeWidth > result || !canOverride)
      result = themeWidth;
  }

#ifdef DEBUG_INTRINSIC_WIDTH
  nsFrame::IndentBy(stdout, gNoiseIndent);
  static_cast<nsFrame*>(aFrame)->ListTag(stdout);
  printf(" %s intrinsic width for container is %d twips.\n",
         aType == MIN_WIDTH ? "min" : "pref", result);
#endif

  return result;
}

 nscoord
nsLayoutUtils::ComputeWidthDependentValue(
                 nscoord              aContainingBlockWidth,
                 const nsStyleCoord&  aCoord)
{
  NS_WARN_IF_FALSE(aContainingBlockWidth != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");

  if (aCoord.IsCoordPercentCalcUnit()) {
    return nsRuleNode::ComputeCoordPercentCalc(aCoord, aContainingBlockWidth);
  }
  NS_ASSERTION(aCoord.GetUnit() == eStyleUnit_None ||
               aCoord.GetUnit() == eStyleUnit_Auto,
               "unexpected width value");
  return 0;
}

 nscoord
nsLayoutUtils::ComputeWidthValue(
                 nsRenderingContext* aRenderingContext,
                 nsIFrame*            aFrame,
                 nscoord              aContainingBlockWidth,
                 nscoord              aContentEdgeToBoxSizing,
                 nscoord              aBoxSizingToMarginEdge,
                 const nsStyleCoord&  aCoord)
{
  NS_PRECONDITION(aFrame, "non-null frame expected");
  NS_PRECONDITION(aRenderingContext, "non-null rendering context expected");
  NS_WARN_IF_FALSE(aContainingBlockWidth != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
  NS_PRECONDITION(aContainingBlockWidth >= 0,
                  "width less than zero");

  nscoord result;
  if (aCoord.IsCoordPercentCalcUnit()) {
    result = nsRuleNode::ComputeCoordPercentCalc(aCoord, aContainingBlockWidth);
    
    
    
    result -= aContentEdgeToBoxSizing;
  } else if (eStyleUnit_Enumerated == aCoord.GetUnit()) {
    
    
    AutoMaybeDisableFontInflation an(aFrame);

    PRInt32 val = aCoord.GetIntValue();
    switch (val) {
      case NS_STYLE_WIDTH_MAX_CONTENT:
        result = aFrame->GetPrefWidth(aRenderingContext);
        NS_ASSERTION(result >= 0, "width less than zero");
        break;
      case NS_STYLE_WIDTH_MIN_CONTENT:
        result = aFrame->GetMinWidth(aRenderingContext);
        NS_ASSERTION(result >= 0, "width less than zero");
        break;
      case NS_STYLE_WIDTH_FIT_CONTENT:
        {
          nscoord pref = aFrame->GetPrefWidth(aRenderingContext),
                   min = aFrame->GetMinWidth(aRenderingContext),
                  fill = aContainingBlockWidth -
                         (aBoxSizingToMarginEdge + aContentEdgeToBoxSizing);
          result = NS_MAX(min, NS_MIN(pref, fill));
          NS_ASSERTION(result >= 0, "width less than zero");
        }
        break;
      case NS_STYLE_WIDTH_AVAILABLE:
        result = aContainingBlockWidth -
                 (aBoxSizingToMarginEdge + aContentEdgeToBoxSizing);
    }
  } else {
    NS_NOTREACHED("unexpected width value");
    result = 0;
  }
  if (result < 0)
    result = 0;
  return result;
}


 nscoord
nsLayoutUtils::ComputeHeightDependentValue(
                 nscoord              aContainingBlockHeight,
                 const nsStyleCoord&  aCoord)
{
  
  
  
  
  
  
  
  NS_PRECONDITION(NS_AUTOHEIGHT != aContainingBlockHeight ||
                  !aCoord.HasPercent(),
                  "unexpected containing block height");

  if (aCoord.IsCoordPercentCalcUnit()) {
    return nsRuleNode::ComputeCoordPercentCalc(aCoord, aContainingBlockHeight);
  }

  NS_ASSERTION(aCoord.GetUnit() == eStyleUnit_None ||
               aCoord.GetUnit() == eStyleUnit_Auto,
               "unexpected height value");
  return 0;
}

#define MULDIV(a,b,c) (nscoord(PRInt64(a) * PRInt64(b) / PRInt64(c)))

 nsSize
nsLayoutUtils::ComputeSizeWithIntrinsicDimensions(
                   nsRenderingContext* aRenderingContext, nsIFrame* aFrame,
                   const nsIFrame::IntrinsicSize& aIntrinsicSize,
                   nsSize aIntrinsicRatio, nsSize aCBSize,
                   nsSize aMargin, nsSize aBorder, nsSize aPadding)
{
  const nsStylePosition *stylePos = aFrame->GetStylePosition();
  
  
  

  
  
  

  const bool isAutoWidth = stylePos->mWidth.GetUnit() == eStyleUnit_Auto;
  const bool isAutoHeight = IsAutoHeight(stylePos->mHeight, aCBSize.height);

  nsSize boxSizingAdjust(0,0);
  switch (stylePos->mBoxSizing) {
    case NS_STYLE_BOX_SIZING_BORDER:
      boxSizingAdjust += aBorder;
      
    case NS_STYLE_BOX_SIZING_PADDING:
      boxSizingAdjust += aPadding;
  }
  nscoord boxSizingToMarginEdgeWidth =
    aMargin.width + aBorder.width + aPadding.width - boxSizingAdjust.width;

  nscoord width, minWidth, maxWidth, height, minHeight, maxHeight;

  if (!isAutoWidth) {
    width = nsLayoutUtils::ComputeWidthValue(aRenderingContext,
              aFrame, aCBSize.width, boxSizingAdjust.width,
              boxSizingToMarginEdgeWidth, stylePos->mWidth);
    NS_ASSERTION(width >= 0, "negative result from ComputeWidthValue");
  }

  if (stylePos->mMaxWidth.GetUnit() != eStyleUnit_None) {
    maxWidth = nsLayoutUtils::ComputeWidthValue(aRenderingContext,
                 aFrame, aCBSize.width, boxSizingAdjust.width,
                 boxSizingToMarginEdgeWidth, stylePos->mMaxWidth);
    NS_ASSERTION(maxWidth >= 0, "negative result from ComputeWidthValue");
  } else {
    maxWidth = nscoord_MAX;
  }

  minWidth = nsLayoutUtils::ComputeWidthValue(aRenderingContext,
               aFrame, aCBSize.width, boxSizingAdjust.width,
               boxSizingToMarginEdgeWidth, stylePos->mMinWidth);
  NS_ASSERTION(minWidth >= 0, "negative result from ComputeWidthValue");

  if (!isAutoHeight) {
    height = nsLayoutUtils::
      ComputeHeightValue(aCBSize.height, stylePos->mHeight) -
      boxSizingAdjust.height;
    if (height < 0)
      height = 0;
  }

  if (!IsAutoHeight(stylePos->mMaxHeight, aCBSize.height)) {
    maxHeight = nsLayoutUtils::
      ComputeHeightValue(aCBSize.height, stylePos->mMaxHeight) -
      boxSizingAdjust.height;
    if (maxHeight < 0)
      maxHeight = 0;
  } else {
    maxHeight = nscoord_MAX;
  }

  if (!IsAutoHeight(stylePos->mMinHeight, aCBSize.height)) {
    minHeight = nsLayoutUtils::
      ComputeHeightValue(aCBSize.height, stylePos->mMinHeight) -
      boxSizingAdjust.height;
    if (minHeight < 0)
      minHeight = 0;
  } else {
    minHeight = 0;
  }

  

  NS_ASSERTION(aCBSize.width != NS_UNCONSTRAINEDSIZE,
               "Our containing block must not have unconstrained width!");

  bool hasIntrinsicWidth, hasIntrinsicHeight;
  nscoord intrinsicWidth, intrinsicHeight;

  if (aIntrinsicSize.width.GetUnit() == eStyleUnit_Coord) {
    hasIntrinsicWidth = true;
    intrinsicWidth = aIntrinsicSize.width.GetCoordValue();
    if (intrinsicWidth < 0)
      intrinsicWidth = 0;
  } else {
    NS_ASSERTION(aIntrinsicSize.width.GetUnit() == eStyleUnit_None,
                 "unexpected unit");
    hasIntrinsicWidth = false;
    intrinsicWidth = 0;
  }

  if (aIntrinsicSize.height.GetUnit() == eStyleUnit_Coord) {
    hasIntrinsicHeight = true;
    intrinsicHeight = aIntrinsicSize.height.GetCoordValue();
    if (intrinsicHeight < 0)
      intrinsicHeight = 0;
  } else {
    NS_ASSERTION(aIntrinsicSize.height.GetUnit() == eStyleUnit_None,
                 "unexpected unit");
    hasIntrinsicHeight = false;
    intrinsicHeight = 0;
  }

  NS_ASSERTION(aIntrinsicRatio.width >= 0 && aIntrinsicRatio.height >= 0,
               "Intrinsic ratio has a negative component!");

  

  if (isAutoWidth) {
    if (isAutoHeight) {

      

      

      nscoord tentWidth, tentHeight;

      if (hasIntrinsicWidth) {
        tentWidth = intrinsicWidth;
      } else if (hasIntrinsicHeight && aIntrinsicRatio.height > 0) {
        tentWidth = MULDIV(intrinsicHeight, aIntrinsicRatio.width, aIntrinsicRatio.height);
      } else if (aIntrinsicRatio.width > 0) {
        tentWidth = aCBSize.width - boxSizingToMarginEdgeWidth; 
        if (tentWidth < 0) tentWidth = 0;
      } else {
        tentWidth = nsPresContext::CSSPixelsToAppUnits(300);
      }

      if (hasIntrinsicHeight) {
        tentHeight = intrinsicHeight;
      } else if (aIntrinsicRatio.width > 0) {
        tentHeight = MULDIV(tentWidth, aIntrinsicRatio.height, aIntrinsicRatio.width);
      } else {
        tentHeight = nsPresContext::CSSPixelsToAppUnits(150);
      }

      return ComputeAutoSizeWithIntrinsicDimensions(minWidth, minHeight,
                                                    maxWidth, maxHeight,
                                                    tentWidth, tentHeight);
    } else {

      
      height = NS_CSS_MINMAX(height, minHeight, maxHeight);
      if (aIntrinsicRatio.height > 0) {
        width = MULDIV(height, aIntrinsicRatio.width, aIntrinsicRatio.height);
      } else if (hasIntrinsicWidth) {
        width = intrinsicWidth;
      } else {
        width = nsPresContext::CSSPixelsToAppUnits(300);
      }
      width = NS_CSS_MINMAX(width, minWidth, maxWidth);

    }
  } else {
    if (isAutoHeight) {

      
      width = NS_CSS_MINMAX(width, minWidth, maxWidth);
      if (aIntrinsicRatio.width > 0) {
        height = MULDIV(width, aIntrinsicRatio.height, aIntrinsicRatio.width);
      } else if (hasIntrinsicHeight) {
        height = intrinsicHeight;
      } else {
        height = nsPresContext::CSSPixelsToAppUnits(150);
      }
      height = NS_CSS_MINMAX(height, minHeight, maxHeight);

    } else {

      
      width = NS_CSS_MINMAX(width, minWidth, maxWidth);
      height = NS_CSS_MINMAX(height, minHeight, maxHeight);

    }
  }

  return nsSize(width, height);
}

nsSize
nsLayoutUtils::ComputeAutoSizeWithIntrinsicDimensions(nscoord minWidth, nscoord minHeight,
                                                      nscoord maxWidth, nscoord maxHeight,
                                                      nscoord tentWidth, nscoord tentHeight)
{
  

  if (minWidth > maxWidth)
    maxWidth = minWidth;
  if (minHeight > maxHeight)
    maxHeight = minHeight;

  nscoord heightAtMaxWidth, heightAtMinWidth,
          widthAtMaxHeight, widthAtMinHeight;

  if (tentWidth > 0) {
    heightAtMaxWidth = MULDIV(maxWidth, tentHeight, tentWidth);
    if (heightAtMaxWidth < minHeight)
      heightAtMaxWidth = minHeight;
    heightAtMinWidth = MULDIV(minWidth, tentHeight, tentWidth);
    if (heightAtMinWidth > maxHeight)
      heightAtMinWidth = maxHeight;
  } else {
    heightAtMaxWidth = heightAtMinWidth = NS_CSS_MINMAX(tentHeight, minHeight, maxHeight);
  }

  if (tentHeight > 0) {
    widthAtMaxHeight = MULDIV(maxHeight, tentWidth, tentHeight);
    if (widthAtMaxHeight < minWidth)
      widthAtMaxHeight = minWidth;
    widthAtMinHeight = MULDIV(minHeight, tentWidth, tentHeight);
    if (widthAtMinHeight > maxWidth)
      widthAtMinHeight = maxWidth;
  } else {
    widthAtMaxHeight = widthAtMinHeight = NS_CSS_MINMAX(tentWidth, minWidth, maxWidth);
  }

  

  nscoord width, height;

  if (tentWidth > maxWidth) {
    if (tentHeight > maxHeight) {
      if (PRInt64(maxWidth) * PRInt64(tentHeight) <=
          PRInt64(maxHeight) * PRInt64(tentWidth)) {
        width = maxWidth;
        height = heightAtMaxWidth;
      } else {
        width = widthAtMaxHeight;
        height = maxHeight;
      }
    } else {
      
      
      
      width = maxWidth;
      height = heightAtMaxWidth;
    }
  } else if (tentWidth < minWidth) {
    if (tentHeight < minHeight) {
      if (PRInt64(minWidth) * PRInt64(tentHeight) <=
          PRInt64(minHeight) * PRInt64(tentWidth)) {
        width = widthAtMinHeight;
        height = minHeight;
      } else {
        width = minWidth;
        height = heightAtMinWidth;
      }
    } else {
      
      
      
      width = minWidth;
      height = heightAtMinWidth;
    }
  } else {
    if (tentHeight > maxHeight) {
      width = widthAtMaxHeight;
      height = maxHeight;
    } else if (tentHeight < minHeight) {
      width = widthAtMinHeight;
      height = minHeight;
    } else {
      width = tentWidth;
      height = tentHeight;
    }
  }

  return nsSize(width, height);
}

 nscoord
nsLayoutUtils::MinWidthFromInline(nsIFrame* aFrame,
                                  nsRenderingContext* aRenderingContext)
{
  NS_ASSERTION(!nsLayoutUtils::IsContainerForFontSizeInflation(aFrame),
               "should not be container for font size inflation");

  nsIFrame::InlineMinWidthData data;
  DISPLAY_MIN_WIDTH(aFrame, data.prevLines);
  aFrame->AddInlineMinWidth(aRenderingContext, &data);
  data.ForceBreak(aRenderingContext);
  return data.prevLines;
}

 nscoord
nsLayoutUtils::PrefWidthFromInline(nsIFrame* aFrame,
                                   nsRenderingContext* aRenderingContext)
{
  NS_ASSERTION(!nsLayoutUtils::IsContainerForFontSizeInflation(aFrame),
               "should not be container for font size inflation");

  nsIFrame::InlinePrefWidthData data;
  DISPLAY_PREF_WIDTH(aFrame, data.prevLines);
  aFrame->AddInlinePrefWidth(aRenderingContext, &data);
  data.ForceBreak(aRenderingContext);
  return data.prevLines;
}

static nscolor
DarkenColor(nscolor aColor)
{
  PRUint16  hue, sat, value;
  PRUint8 alpha;

  
  NS_RGB2HSV(aColor, hue, sat, value, alpha);

  
  
  
  
  
  
  if (value > sat) {
    value = sat;
    
    NS_HSV2RGB(aColor, hue, sat, value, alpha);
  }
  return aColor;
}




static bool
ShouldDarkenColors(nsPresContext* aPresContext)
{
  return !aPresContext->GetBackgroundColorDraw() &&
         !aPresContext->GetBackgroundImageDraw();
}

nscolor
nsLayoutUtils::GetColor(nsIFrame* aFrame, nsCSSProperty aProperty)
{
  nscolor color = aFrame->GetVisitedDependentColor(aProperty);
  if (ShouldDarkenColors(aFrame->PresContext())) {
    color = DarkenColor(color);
  }
  return color;
}

gfxFloat
nsLayoutUtils::GetSnappedBaselineY(nsIFrame* aFrame, gfxContext* aContext,
                                   nscoord aY, nscoord aAscent)
{
  gfxFloat appUnitsPerDevUnit = aFrame->PresContext()->AppUnitsPerDevPixel();
  gfxFloat baseline = gfxFloat(aY) + aAscent;
  gfxRect putativeRect(0, baseline/appUnitsPerDevUnit, 1, 1);
  if (!aContext->UserToDevicePixelSnapped(putativeRect, true))
    return baseline;
  return aContext->DeviceToUser(putativeRect.TopLeft()).y * appUnitsPerDevUnit;
}

void
nsLayoutUtils::DrawString(const nsIFrame*      aFrame,
                          nsRenderingContext* aContext,
                          const PRUnichar*     aString,
                          PRInt32              aLength,
                          nsPoint              aPoint,
                          PRUint8              aDirection)
{
#ifdef IBMBIDI
  nsresult rv = NS_ERROR_FAILURE;
  nsPresContext* presContext = aFrame->PresContext();
  if (presContext->BidiEnabled()) {
    if (aDirection == NS_STYLE_DIRECTION_INHERIT) {
      aDirection = aFrame->GetStyleVisibility()->mDirection;
    }
    nsBidiDirection direction =
      (NS_STYLE_DIRECTION_RTL == aDirection) ?
      NSBIDI_RTL : NSBIDI_LTR;
    rv = nsBidiPresUtils::RenderText(aString, aLength, direction,
                                     presContext, *aContext, *aContext,
                                     aPoint.x, aPoint.y);
  }
  if (NS_FAILED(rv))
#endif 
  {
    aContext->SetTextRunRTL(false);
    aContext->DrawString(aString, aLength, aPoint.x, aPoint.y);
  }
}

nscoord
nsLayoutUtils::GetStringWidth(const nsIFrame*      aFrame,
                              nsRenderingContext* aContext,
                              const PRUnichar*     aString,
                              PRInt32              aLength)
{
#ifdef IBMBIDI
  nsPresContext* presContext = aFrame->PresContext();
  if (presContext->BidiEnabled()) {
    const nsStyleVisibility* vis = aFrame->GetStyleVisibility();
    nsBidiDirection direction =
      (NS_STYLE_DIRECTION_RTL == vis->mDirection) ?
      NSBIDI_RTL : NSBIDI_LTR;
    return nsBidiPresUtils::MeasureTextWidth(aString, aLength,
                                             direction, presContext, *aContext);
  }
#endif 
  aContext->SetTextRunRTL(false);
  return aContext->GetWidth(aString, aLength);
}

 void
nsLayoutUtils::PaintTextShadow(const nsIFrame* aFrame,
                               nsRenderingContext* aContext,
                               const nsRect& aTextRect,
                               const nsRect& aDirtyRect,
                               const nscolor& aForegroundColor,
                               TextShadowCallback aCallback,
                               void* aCallbackData)
{
  const nsStyleText* textStyle = aFrame->GetStyleText();
  if (!textStyle->mTextShadow)
    return;

  
  
  gfxContext* aDestCtx = aContext->ThebesContext();
  for (PRUint32 i = textStyle->mTextShadow->Length(); i > 0; --i) {
    nsCSSShadowItem* shadowDetails = textStyle->mTextShadow->ShadowAt(i - 1);
    nsPoint shadowOffset(shadowDetails->mXOffset,
                         shadowDetails->mYOffset);
    nscoord blurRadius = NS_MAX(shadowDetails->mRadius, 0);

    nsRect shadowRect(aTextRect);
    shadowRect.MoveBy(shadowOffset);

    nsPresContext* presCtx = aFrame->PresContext();
    nsContextBoxBlur contextBoxBlur;
    gfxContext* shadowContext = contextBoxBlur.Init(shadowRect, 0, blurRadius,
                                                    presCtx->AppUnitsPerDevPixel(),
                                                    aDestCtx, aDirtyRect, nullptr);
    if (!shadowContext)
      continue;

    nscolor shadowColor;
    if (shadowDetails->mHasColor)
      shadowColor = shadowDetails->mColor;
    else
      shadowColor = aForegroundColor;

    
    
    nsRefPtr<nsRenderingContext> renderingContext = new nsRenderingContext();
    renderingContext->Init(presCtx->DeviceContext(), shadowContext);

    aDestCtx->Save();
    aDestCtx->NewPath();
    aDestCtx->SetColor(gfxRGBA(shadowColor));

    
    aCallback(renderingContext, shadowOffset, shadowColor, aCallbackData);

    contextBoxBlur.DoPaint();
    aDestCtx->Restore();
  }
}

 nscoord
nsLayoutUtils::GetCenteredFontBaseline(nsFontMetrics* aFontMetrics,
                                       nscoord         aLineHeight)
{
  nscoord fontAscent = aFontMetrics->MaxAscent();
  nscoord fontHeight = aFontMetrics->MaxHeight();

  nscoord leading = aLineHeight - fontHeight;
  return fontAscent + leading/2;
}


 bool
nsLayoutUtils::GetFirstLineBaseline(const nsIFrame* aFrame, nscoord* aResult)
{
  LinePosition position;
  if (!GetFirstLinePosition(aFrame, &position))
    return false;
  *aResult = position.mBaseline;
  return true;
}

 bool
nsLayoutUtils::GetFirstLinePosition(const nsIFrame* aFrame,
                                    LinePosition* aResult)
{
  const nsBlockFrame* block = nsLayoutUtils::GetAsBlock(const_cast<nsIFrame*>(aFrame));
  if (!block) {
    
    
    nsIAtom* fType = aFrame->GetType();
    if (fType == nsGkAtoms::tableOuterFrame) {
      aResult->mTop = 0;
      aResult->mBaseline = aFrame->GetBaseline();
      
      
      aResult->mBottom = aFrame->GetSize().height;
      return true;
    }

    
    if (fType == nsGkAtoms::scrollFrame) {
      nsIScrollableFrame *sFrame = do_QueryFrame(const_cast<nsIFrame*>(aFrame));
      if (!sFrame) {
        NS_NOTREACHED("not scroll frame");
      }
      LinePosition kidPosition;
      if (GetFirstLinePosition(sFrame->GetScrolledFrame(), &kidPosition)) {
        
        
        
        *aResult = kidPosition + aFrame->GetUsedBorderAndPadding().top;
        return true;
      }
      return false;
    }

    if (fType == nsGkAtoms::fieldSetFrame) {
      LinePosition kidPosition;
      nsIFrame* kid = aFrame->GetFirstPrincipalChild();
      
      if (GetFirstLinePosition(kid, &kidPosition)) {
        *aResult = kidPosition + kid->GetPosition().y;
        return true;
      }
      return false;
    }

    
    return false;
  }

  for (nsBlockFrame::const_line_iterator line = block->begin_lines(),
                                     line_end = block->end_lines();
       line != line_end; ++line) {
    if (line->IsBlock()) {
      nsIFrame *kid = line->mFirstChild;
      LinePosition kidPosition;
      if (GetFirstLinePosition(kid, &kidPosition)) {
        *aResult = kidPosition + kid->GetPosition().y;
        return true;
      }
    } else {
      
      
      if (line->GetHeight() != 0 || !line->IsEmpty()) {
        nscoord top = line->mBounds.y;
        aResult->mTop = top;
        aResult->mBaseline = top + line->GetAscent();
        aResult->mBottom = top + line->GetHeight();
        return true;
      }
    }
  }
  return false;
}

 bool
nsLayoutUtils::GetLastLineBaseline(const nsIFrame* aFrame, nscoord* aResult)
{
  const nsBlockFrame* block = nsLayoutUtils::GetAsBlock(const_cast<nsIFrame*>(aFrame));
  if (!block)
    
    return false;

  for (nsBlockFrame::const_reverse_line_iterator line = block->rbegin_lines(),
                                             line_end = block->rend_lines();
       line != line_end; ++line) {
    if (line->IsBlock()) {
      nsIFrame *kid = line->mFirstChild;
      nscoord kidBaseline;
      if (GetLastLineBaseline(kid, &kidBaseline)) {
        *aResult = kidBaseline + kid->GetPosition().y;
        return true;
      } else if (kid->GetType() == nsGkAtoms::scrollFrame) {
        
        
        *aResult = kid->GetRect().YMost();
        return true;
      }
    } else {
      
      
      if (line->GetHeight() != 0 || !line->IsEmpty()) {
        *aResult = line->mBounds.y + line->GetAscent();
        return true;
      }
    }
  }
  return false;
}

static nscoord
CalculateBlockContentBottom(nsBlockFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null ptr");

  nscoord contentBottom = 0;

  for (nsBlockFrame::line_iterator line = aFrame->begin_lines(),
                                   line_end = aFrame->end_lines();
       line != line_end; ++line) {
    if (line->IsBlock()) {
      nsIFrame* child = line->mFirstChild;
      nscoord offset = child->GetRect().y - child->GetRelativeOffset().y;
      contentBottom = NS_MAX(contentBottom,
                        nsLayoutUtils::CalculateContentBottom(child) + offset);
    }
    else {
      contentBottom = NS_MAX(contentBottom, line->mBounds.YMost());
    }
  }
  return contentBottom;
}

 nscoord
nsLayoutUtils::CalculateContentBottom(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null ptr");

  nscoord contentBottom = aFrame->GetRect().height;

  
  
  if (aFrame->GetScrollableOverflowRect().height > contentBottom) {
    nsIFrame::ChildListIDs skip(nsIFrame::kOverflowList |
                                nsIFrame::kExcessOverflowContainersList |
                                nsIFrame::kOverflowOutOfFlowList);
    nsBlockFrame* blockFrame = GetAsBlock(aFrame);
    if (blockFrame) {
      contentBottom =
        NS_MAX(contentBottom, CalculateBlockContentBottom(blockFrame));
      skip |= nsIFrame::kPrincipalList;
    }
    nsIFrame::ChildListIterator lists(aFrame);
    for (; !lists.IsDone(); lists.Next()) {
      if (!skip.Contains(lists.CurrentID())) {
        nsFrameList::Enumerator childFrames(lists.CurrentList()); 
        for (; !childFrames.AtEnd(); childFrames.Next()) {
          nsIFrame* child = childFrames.get();
          nscoord offset = child->GetRect().y - child->GetRelativeOffset().y;
          contentBottom = NS_MAX(contentBottom,
                                 CalculateContentBottom(child) + offset);
        }
      }
    }
  }
  return contentBottom;
}

 nsIFrame*
nsLayoutUtils::GetClosestLayer(nsIFrame* aFrame)
{
  nsIFrame* layer;
  for (layer = aFrame; layer; layer = layer->GetParent()) {
    if (layer->IsPositioned() ||
        (layer->GetParent() &&
          layer->GetParent()->GetType() == nsGkAtoms::scrollFrame))
      break;
  }
  if (layer)
    return layer;
  return aFrame->PresContext()->PresShell()->FrameManager()->GetRootFrame();
}

GraphicsFilter
nsLayoutUtils::GetGraphicsFilterForFrame(nsIFrame* aForFrame)
{
  GraphicsFilter defaultFilter = gfxPattern::FILTER_GOOD;
  nsIFrame *frame = nsCSSRendering::IsCanvasFrame(aForFrame) ?
    nsCSSRendering::FindBackgroundStyleFrame(aForFrame) : aForFrame;

  switch (frame->GetStyleSVG()->mImageRendering) {
  case NS_STYLE_IMAGE_RENDERING_OPTIMIZESPEED:
    return gfxPattern::FILTER_FAST;
  case NS_STYLE_IMAGE_RENDERING_OPTIMIZEQUALITY:
    return gfxPattern::FILTER_BEST;
  case NS_STYLE_IMAGE_RENDERING_CRISPEDGES:
    return gfxPattern::FILTER_NEAREST;
  default:
    return defaultFilter;
  }
}









static gfxPoint
MapToFloatImagePixels(const gfxSize& aSize,
                      const gfxRect& aDest, const gfxPoint& aPt)
{
  return gfxPoint(((aPt.x - aDest.X())*aSize.width)/aDest.Width(),
                  ((aPt.y - aDest.Y())*aSize.height)/aDest.Height());
}









static gfxPoint
MapToFloatUserPixels(const gfxSize& aSize,
                     const gfxRect& aDest, const gfxPoint& aPt)
{
  return gfxPoint(aPt.x*aDest.Width()/aSize.width + aDest.X(),
                  aPt.y*aDest.Height()/aSize.height + aDest.Y());
}

 gfxRect
nsLayoutUtils::RectToGfxRect(const nsRect& aRect, PRInt32 aAppUnitsPerDevPixel)
{
  return gfxRect(gfxFloat(aRect.x) / aAppUnitsPerDevPixel,
                 gfxFloat(aRect.y) / aAppUnitsPerDevPixel,
                 gfxFloat(aRect.width) / aAppUnitsPerDevPixel,
                 gfxFloat(aRect.height) / aAppUnitsPerDevPixel);
}

struct SnappedImageDrawingParameters {
  
  
  gfxMatrix mUserSpaceToImageSpace;
  
  gfxRect mFillRect;
  
  
  nsIntRect mSubimage;
  
  bool mShouldDraw;
  
  
  bool mResetCTM;

  SnappedImageDrawingParameters()
   : mShouldDraw(false)
   , mResetCTM(false)
  {}

  SnappedImageDrawingParameters(const gfxMatrix& aUserSpaceToImageSpace,
                                const gfxRect&   aFillRect,
                                const nsIntRect& aSubimage,
                                bool             aResetCTM)
   : mUserSpaceToImageSpace(aUserSpaceToImageSpace)
   , mFillRect(aFillRect)
   , mSubimage(aSubimage)
   , mShouldDraw(true)
   , mResetCTM(aResetCTM)
  {}
};








static SnappedImageDrawingParameters
ComputeSnappedImageDrawingParameters(gfxContext*     aCtx,
                                     PRInt32         aAppUnitsPerDevPixel,
                                     const nsRect    aDest,
                                     const nsRect    aFill,
                                     const nsPoint   aAnchor,
                                     const nsRect    aDirty,
                                     const nsIntSize aImageSize)

{
  if (aDest.IsEmpty() || aFill.IsEmpty())
    return SnappedImageDrawingParameters();

  gfxRect devPixelDest =
    nsLayoutUtils::RectToGfxRect(aDest, aAppUnitsPerDevPixel);
  gfxRect devPixelFill =
    nsLayoutUtils::RectToGfxRect(aFill, aAppUnitsPerDevPixel);
  gfxRect devPixelDirty =
    nsLayoutUtils::RectToGfxRect(aDirty, aAppUnitsPerDevPixel);

  bool ignoreScale = false;
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
  ignoreScale = true;
#endif
  gfxRect fill = devPixelFill;
  bool didSnap = aCtx->UserToDevicePixelSnapped(fill, ignoreScale);

  gfxSize imageSize(aImageSize.width, aImageSize.height);

  
  gfxPoint subimageTopLeft =
    MapToFloatImagePixels(imageSize, devPixelDest, devPixelFill.TopLeft());
  gfxPoint subimageBottomRight =
    MapToFloatImagePixels(imageSize, devPixelDest, devPixelFill.BottomRight());
  nsIntRect intSubimage;
  intSubimage.MoveTo(NSToIntFloor(subimageTopLeft.x),
                     NSToIntFloor(subimageTopLeft.y));
  intSubimage.SizeTo(NSToIntCeil(subimageBottomRight.x) - intSubimage.x,
                     NSToIntCeil(subimageBottomRight.y) - intSubimage.y);

  
  
  
  gfxPoint anchorPoint(gfxFloat(aAnchor.x)/aAppUnitsPerDevPixel,
                       gfxFloat(aAnchor.y)/aAppUnitsPerDevPixel);
  gfxPoint imageSpaceAnchorPoint =
    MapToFloatImagePixels(imageSize, devPixelDest, anchorPoint);
  gfxMatrix currentMatrix = aCtx->CurrentMatrix();

  if (didSnap) {
    NS_ASSERTION(!currentMatrix.HasNonAxisAlignedTransform(),
                 "How did we snap, then?");
    imageSpaceAnchorPoint.Round();
    anchorPoint = imageSpaceAnchorPoint;
    anchorPoint = MapToFloatUserPixels(imageSize, devPixelDest, anchorPoint);
    anchorPoint = currentMatrix.Transform(anchorPoint);
    anchorPoint.Round();

    
    
    devPixelDirty = currentMatrix.Transform(devPixelDirty);
  }

  gfxFloat scaleX = imageSize.width*aAppUnitsPerDevPixel/aDest.width;
  gfxFloat scaleY = imageSize.height*aAppUnitsPerDevPixel/aDest.height;
  if (didSnap) {
    
    
    scaleX /= currentMatrix.xx;
    scaleY /= currentMatrix.yy;
  }
  gfxFloat translateX = imageSpaceAnchorPoint.x - anchorPoint.x*scaleX;
  gfxFloat translateY = imageSpaceAnchorPoint.y - anchorPoint.y*scaleY;
  gfxMatrix transform(scaleX, 0, 0, scaleY, translateX, translateY);

  gfxRect finalFillRect = fill;
  
  
  
  
  
  
  
  if (didSnap && !transform.HasNonIntegerTranslation()) {
    devPixelDirty.RoundOut();
    finalFillRect = fill.Intersect(devPixelDirty);
  }
  if (finalFillRect.IsEmpty())
    return SnappedImageDrawingParameters();

  return SnappedImageDrawingParameters(transform, finalFillRect, intSubimage,
                                       didSnap);
}


static nsresult
DrawImageInternal(nsRenderingContext* aRenderingContext,
                  imgIContainer*       aImage,
                  GraphicsFilter       aGraphicsFilter,
                  const nsRect&        aDest,
                  const nsRect&        aFill,
                  const nsPoint&       aAnchor,
                  const nsRect&        aDirty,
                  const nsIntSize&     aImageSize,
                  PRUint32             aImageFlags)
{
  if (aDest.Contains(aFill)) {
    aImageFlags |= imgIContainer::FLAG_CLAMP;
  }
  PRInt32 appUnitsPerDevPixel = aRenderingContext->AppUnitsPerDevPixel();
  gfxContext* ctx = aRenderingContext->ThebesContext();

  SnappedImageDrawingParameters drawingParams =
    ComputeSnappedImageDrawingParameters(ctx, appUnitsPerDevPixel, aDest, aFill,
                                         aAnchor, aDirty, aImageSize);

  if (!drawingParams.mShouldDraw)
    return NS_OK;

  gfxContextMatrixAutoSaveRestore saveMatrix(ctx);
  if (drawingParams.mResetCTM) {
    ctx->IdentityMatrix();
  }

  aImage->Draw(ctx, aGraphicsFilter, drawingParams.mUserSpaceToImageSpace,
               drawingParams.mFillRect, drawingParams.mSubimage, aImageSize,
               aImageFlags);
  return NS_OK;
}

 void
nsLayoutUtils::DrawPixelSnapped(nsRenderingContext* aRenderingContext,
                                gfxDrawable*         aDrawable,
                                GraphicsFilter       aFilter,
                                const nsRect&        aDest,
                                const nsRect&        aFill,
                                const nsPoint&       aAnchor,
                                const nsRect&        aDirty)
{
  PRInt32 appUnitsPerDevPixel = aRenderingContext->AppUnitsPerDevPixel();
  gfxContext* ctx = aRenderingContext->ThebesContext();
  gfxIntSize drawableSize = aDrawable->Size();
  nsIntSize imageSize(drawableSize.width, drawableSize.height);

  SnappedImageDrawingParameters drawingParams =
    ComputeSnappedImageDrawingParameters(ctx, appUnitsPerDevPixel, aDest, aFill,
                                         aAnchor, aDirty, imageSize);

  if (!drawingParams.mShouldDraw)
    return;

  gfxContextMatrixAutoSaveRestore saveMatrix(ctx);
  if (drawingParams.mResetCTM) {
    ctx->IdentityMatrix();
  }

  gfxRect sourceRect =
    drawingParams.mUserSpaceToImageSpace.Transform(drawingParams.mFillRect);
  gfxRect imageRect(0, 0, imageSize.width, imageSize.height);
  gfxRect subimage(drawingParams.mSubimage.x, drawingParams.mSubimage.y,
                   drawingParams.mSubimage.width, drawingParams.mSubimage.height);

  NS_ASSERTION(!sourceRect.Intersect(subimage).IsEmpty(),
               "We must be allowed to sample *some* source pixels!");

  gfxUtils::DrawPixelSnapped(ctx, aDrawable,
                             drawingParams.mUserSpaceToImageSpace, subimage,
                             sourceRect, imageRect, drawingParams.mFillRect,
                             gfxASurface::ImageFormatARGB32, aFilter);
}

 nsresult
nsLayoutUtils::DrawSingleUnscaledImage(nsRenderingContext* aRenderingContext,
                                       imgIContainer*       aImage,
                                       GraphicsFilter       aGraphicsFilter,
                                       const nsPoint&       aDest,
                                       const nsRect*        aDirty,
                                       PRUint32             aImageFlags,
                                       const nsRect*        aSourceArea)
{
  nsIntSize imageSize;
  aImage->GetWidth(&imageSize.width);
  aImage->GetHeight(&imageSize.height);
  NS_ENSURE_TRUE(imageSize.width > 0 && imageSize.height > 0, NS_ERROR_FAILURE);

  nscoord appUnitsPerCSSPixel = nsDeviceContext::AppUnitsPerCSSPixel();
  nsSize size(imageSize.width*appUnitsPerCSSPixel,
              imageSize.height*appUnitsPerCSSPixel);

  nsRect source;
  if (aSourceArea) {
    source = *aSourceArea;
  } else {
    source.SizeTo(size);
  }

  nsRect dest(aDest - source.TopLeft(), size);
  nsRect fill(aDest, source.Size());
  
  
  
  fill.IntersectRect(fill, dest);
  return DrawImageInternal(aRenderingContext, aImage, aGraphicsFilter,
                           dest, fill, aDest, aDirty ? *aDirty : dest,
                           imageSize, aImageFlags);
}

 nsresult
nsLayoutUtils::DrawSingleImage(nsRenderingContext* aRenderingContext,
                               imgIContainer*       aImage,
                               GraphicsFilter       aGraphicsFilter,
                               const nsRect&        aDest,
                               const nsRect&        aDirty,
                               PRUint32             aImageFlags,
                               const nsRect*        aSourceArea)
{
  nsIntSize imageSize;
  if (aImage->GetType() == imgIContainer::TYPE_VECTOR) {
    imageSize.width  = nsPresContext::AppUnitsToIntCSSPixels(aDest.width);
    imageSize.height = nsPresContext::AppUnitsToIntCSSPixels(aDest.height);
  } else {
    aImage->GetWidth(&imageSize.width);
    aImage->GetHeight(&imageSize.height);
  }
  NS_ENSURE_TRUE(imageSize.width > 0 && imageSize.height > 0, NS_ERROR_FAILURE);

  nsRect source;
  if (aSourceArea) {
    source = *aSourceArea;
  } else {
    nscoord appUnitsPerCSSPixel = nsDeviceContext::AppUnitsPerCSSPixel();
    source.SizeTo(imageSize.width*appUnitsPerCSSPixel,
                  imageSize.height*appUnitsPerCSSPixel);
  }

  nsRect dest = nsLayoutUtils::GetWholeImageDestination(imageSize, source,
                                                        aDest);
  
  
  
  nsRect fill;
  fill.IntersectRect(aDest, dest);
  return DrawImageInternal(aRenderingContext, aImage, aGraphicsFilter, dest, fill,
                           fill.TopLeft(), aDirty, imageSize, aImageFlags);
}

 void
nsLayoutUtils::ComputeSizeForDrawing(imgIContainer *aImage,
                                     nsIntSize&     aImageSize, 
                                     nsSize&        aIntrinsicRatio, 
                                     bool&          aGotWidth,  
                                     bool&          aGotHeight  )
{
  aGotWidth  = NS_SUCCEEDED(aImage->GetWidth(&aImageSize.width));
  aGotHeight = NS_SUCCEEDED(aImage->GetHeight(&aImageSize.height));

  if (aGotWidth && aGotHeight) {
    aIntrinsicRatio = nsSize(aImageSize.width, aImageSize.height);
    return;
  }

  
  
  
  if (nsIFrame* rootFrame = aImage->GetRootLayoutFrame()) {
    aIntrinsicRatio = rootFrame->GetIntrinsicRatio();
  } else {
    aGotWidth = aGotHeight = true;
    aImageSize = nsIntSize(0, 0);
    aIntrinsicRatio = nsSize(0, 0);
  }
}


 nsresult
nsLayoutUtils::DrawBackgroundImage(nsRenderingContext* aRenderingContext,
                                   imgIContainer*      aImage,
                                   const nsIntSize&    aImageSize,
                                   GraphicsFilter      aGraphicsFilter,
                                   const nsRect&       aDest,
                                   const nsRect&       aFill,
                                   const nsPoint&      aAnchor,
                                   const nsRect&       aDirty,
                                   PRUint32            aImageFlags)
{
  SAMPLE_LABEL("layout", "nsLayoutUtils::DrawBackgroundImage");

  if (UseBackgroundNearestFiltering()) {
    aGraphicsFilter = gfxPattern::FILTER_NEAREST;
  }

  return DrawImageInternal(aRenderingContext, aImage, aGraphicsFilter,
                           aDest, aFill, aAnchor, aDirty,
                           aImageSize, aImageFlags);
}

 nsresult
nsLayoutUtils::DrawImage(nsRenderingContext* aRenderingContext,
                         imgIContainer*       aImage,
                         GraphicsFilter       aGraphicsFilter,
                         const nsRect&        aDest,
                         const nsRect&        aFill,
                         const nsPoint&       aAnchor,
                         const nsRect&        aDirty,
                         PRUint32             aImageFlags)
{
  nsIntSize imageSize;
  nsSize imageRatio;
  bool gotHeight, gotWidth;
  ComputeSizeForDrawing(aImage, imageSize, imageRatio, gotWidth, gotHeight);

  
  
  if (gotWidth != gotHeight) {
    if (!gotWidth) {
      if (imageRatio.height != 0) {
        imageSize.width =
          NSCoordSaturatingNonnegativeMultiply(imageSize.height,
                                               float(imageRatio.width) /
                                               float(imageRatio.height));
        gotWidth = true;
      }
    } else {
      if (imageRatio.width != 0) {
        imageSize.height =
          NSCoordSaturatingNonnegativeMultiply(imageSize.width,
                                               float(imageRatio.height) /
                                               float(imageRatio.width));
        gotHeight = true;
      }
    }
  }

  if (!gotWidth) {
    imageSize.width = nsPresContext::AppUnitsToIntCSSPixels(aFill.width);
  }
  if (!gotHeight) {
    imageSize.height = nsPresContext::AppUnitsToIntCSSPixels(aFill.height);
  }

  return DrawImageInternal(aRenderingContext, aImage, aGraphicsFilter,
                           aDest, aFill, aAnchor, aDirty,
                           imageSize, aImageFlags);
}

 nsRect
nsLayoutUtils::GetWholeImageDestination(const nsIntSize& aWholeImageSize,
                                        const nsRect& aImageSourceArea,
                                        const nsRect& aDestArea)
{
  double scaleX = double(aDestArea.width)/aImageSourceArea.width;
  double scaleY = double(aDestArea.height)/aImageSourceArea.height;
  nscoord destOffsetX = NSToCoordRound(aImageSourceArea.x*scaleX);
  nscoord destOffsetY = NSToCoordRound(aImageSourceArea.y*scaleY);
  nscoord appUnitsPerCSSPixel = nsDeviceContext::AppUnitsPerCSSPixel();
  nscoord wholeSizeX = NSToCoordRound(aWholeImageSize.width*appUnitsPerCSSPixel*scaleX);
  nscoord wholeSizeY = NSToCoordRound(aWholeImageSize.height*appUnitsPerCSSPixel*scaleY);
  return nsRect(aDestArea.TopLeft() - nsPoint(destOffsetX, destOffsetY),
                nsSize(wholeSizeX, wholeSizeY));
}

static bool NonZeroStyleCoord(const nsStyleCoord& aCoord)
{
  if (aCoord.IsCoordPercentCalcUnit()) {
    
    return nsRuleNode::ComputeCoordPercentCalc(aCoord, nscoord_MAX) > 0 ||
           nsRuleNode::ComputeCoordPercentCalc(aCoord, 0) > 0;
  }

  return true;
}

 bool
nsLayoutUtils::HasNonZeroCorner(const nsStyleCorners& aCorners)
{
  NS_FOR_CSS_HALF_CORNERS(corner) {
    if (NonZeroStyleCoord(aCorners.Get(corner)))
      return true;
  }
  return false;
}


static bool IsCornerAdjacentToSide(PRUint8 aCorner, mozilla::css::Side aSide)
{
  PR_STATIC_ASSERT((int)NS_SIDE_TOP == NS_CORNER_TOP_LEFT);
  PR_STATIC_ASSERT((int)NS_SIDE_RIGHT == NS_CORNER_TOP_RIGHT);
  PR_STATIC_ASSERT((int)NS_SIDE_BOTTOM == NS_CORNER_BOTTOM_RIGHT);
  PR_STATIC_ASSERT((int)NS_SIDE_LEFT == NS_CORNER_BOTTOM_LEFT);
  PR_STATIC_ASSERT((int)NS_SIDE_TOP == ((NS_CORNER_TOP_RIGHT - 1)&3));
  PR_STATIC_ASSERT((int)NS_SIDE_RIGHT == ((NS_CORNER_BOTTOM_RIGHT - 1)&3));
  PR_STATIC_ASSERT((int)NS_SIDE_BOTTOM == ((NS_CORNER_BOTTOM_LEFT - 1)&3));
  PR_STATIC_ASSERT((int)NS_SIDE_LEFT == ((NS_CORNER_TOP_LEFT - 1)&3));

  return aSide == aCorner || aSide == ((aCorner - 1)&3);
}

 bool
nsLayoutUtils::HasNonZeroCornerOnSide(const nsStyleCorners& aCorners,
                                      mozilla::css::Side aSide)
{
  PR_STATIC_ASSERT(NS_CORNER_TOP_LEFT_X/2 == NS_CORNER_TOP_LEFT);
  PR_STATIC_ASSERT(NS_CORNER_TOP_LEFT_Y/2 == NS_CORNER_TOP_LEFT);
  PR_STATIC_ASSERT(NS_CORNER_TOP_RIGHT_X/2 == NS_CORNER_TOP_RIGHT);
  PR_STATIC_ASSERT(NS_CORNER_TOP_RIGHT_Y/2 == NS_CORNER_TOP_RIGHT);
  PR_STATIC_ASSERT(NS_CORNER_BOTTOM_RIGHT_X/2 == NS_CORNER_BOTTOM_RIGHT);
  PR_STATIC_ASSERT(NS_CORNER_BOTTOM_RIGHT_Y/2 == NS_CORNER_BOTTOM_RIGHT);
  PR_STATIC_ASSERT(NS_CORNER_BOTTOM_LEFT_X/2 == NS_CORNER_BOTTOM_LEFT);
  PR_STATIC_ASSERT(NS_CORNER_BOTTOM_LEFT_Y/2 == NS_CORNER_BOTTOM_LEFT);

  NS_FOR_CSS_HALF_CORNERS(corner) {
    
    
    if (NonZeroStyleCoord(aCorners.Get(corner)) &&
        IsCornerAdjacentToSide(corner/2, aSide))
      return true;
  }
  return false;
}

 nsTransparencyMode
nsLayoutUtils::GetFrameTransparency(nsIFrame* aBackgroundFrame,
                                    nsIFrame* aCSSRootFrame) {
  if (aCSSRootFrame->GetStyleContext()->GetStyleDisplay()->mOpacity < 1.0f)
    return eTransparencyTransparent;

  if (HasNonZeroCorner(aCSSRootFrame->GetStyleContext()->GetStyleBorder()->mBorderRadius))
    return eTransparencyTransparent;

  if (aCSSRootFrame->GetStyleDisplay()->mAppearance == NS_THEME_WIN_GLASS)
    return eTransparencyGlass;

  if (aCSSRootFrame->GetStyleDisplay()->mAppearance == NS_THEME_WIN_BORDERLESS_GLASS)
    return eTransparencyBorderlessGlass;

  nsITheme::Transparency transparency;
  if (aCSSRootFrame->IsThemed(&transparency))
    return transparency == nsITheme::eTransparent
         ? eTransparencyTransparent
         : eTransparencyOpaque;

  
  
  
  if (aBackgroundFrame->GetType() == nsGkAtoms::viewportFrame &&
      !aBackgroundFrame->GetFirstPrincipalChild()) {
    return eTransparencyOpaque;
  }

  nsStyleContext* bgSC;
  if (!nsCSSRendering::FindBackground(aBackgroundFrame->PresContext(),
                                      aBackgroundFrame, &bgSC)) {
    return eTransparencyTransparent;
  }
  const nsStyleBackground* bg = bgSC->GetStyleBackground();
  if (NS_GET_A(bg->mBackgroundColor) < 255 ||
      
      bg->BottomLayer().mClip != NS_STYLE_BG_CLIP_BORDER)
    return eTransparencyTransparent;
  return eTransparencyOpaque;
}

static bool IsPopupFrame(nsIFrame* aFrame)
{
  
  nsIAtom* frameType = aFrame->GetType();
  if (frameType == nsGkAtoms::listControlFrame) {
    nsListControlFrame* lcf = static_cast<nsListControlFrame*>(aFrame);
    return lcf->IsInDropDownMode();
  }

  
  return frameType == nsGkAtoms::menuPopupFrame;
}

 bool
nsLayoutUtils::IsPopup(nsIFrame* aFrame)
{
  
  if (!aFrame->HasView()) {
    NS_ASSERTION(!IsPopupFrame(aFrame), "popup frame must have a view");
    return false;
  }
  return IsPopupFrame(aFrame);
}

 nsIFrame*
nsLayoutUtils::GetDisplayRootFrame(nsIFrame* aFrame)
{
  nsIFrame* f = aFrame;
  for (;;) {
    if (IsPopup(f))
      return f;
    nsIFrame* parent = GetCrossDocParentFrame(f);
    if (!parent)
      return f;
    f = parent;
  }
}

 PRUint32
nsLayoutUtils::GetTextRunFlagsForStyle(nsStyleContext* aStyleContext,
                                       const nsStyleFont* aStyleFont,
                                       nscoord aLetterSpacing)
{
  PRUint32 result = 0;
  if (aLetterSpacing != 0) {
    result |= gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES;
  }
  switch (aStyleContext->GetStyleSVG()->mTextRendering) {
  case NS_STYLE_TEXT_RENDERING_OPTIMIZESPEED:
    result |= gfxTextRunFactory::TEXT_OPTIMIZE_SPEED;
    break;
  case NS_STYLE_TEXT_RENDERING_AUTO:
    if (aStyleFont->mFont.size <
        aStyleContext->PresContext()->GetAutoQualityMinFontSize()) {
      result |= gfxTextRunFactory::TEXT_OPTIMIZE_SPEED;
    }
    break;
  default:
    break;
  }
  return result;
}

 void
nsLayoutUtils::GetRectDifferenceStrips(const nsRect& aR1, const nsRect& aR2,
                                       nsRect* aHStrip, nsRect* aVStrip) {
  NS_ASSERTION(aR1.TopLeft() == aR2.TopLeft(),
               "expected rects at the same position");
  nsRect unionRect(aR1.x, aR1.y, NS_MAX(aR1.width, aR2.width),
                   NS_MAX(aR1.height, aR2.height));
  nscoord VStripStart = NS_MIN(aR1.width, aR2.width);
  nscoord HStripStart = NS_MIN(aR1.height, aR2.height);
  *aVStrip = unionRect;
  aVStrip->x += VStripStart;
  aVStrip->width -= VStripStart;
  *aHStrip = unionRect;
  aHStrip->y += HStripStart;
  aHStrip->height -= HStripStart;
}

nsDeviceContext*
nsLayoutUtils::GetDeviceContextForScreenInfo(nsPIDOMWindow* aWindow)
{
  if (!aWindow) {
    return nullptr;
  }

  nsCOMPtr<nsIDocShell> docShell = aWindow->GetDocShell();
  while (docShell) {
    
    
    
    
    nsCOMPtr<nsPIDOMWindow> win = do_GetInterface(docShell);
    if (!win) {
      
      return nullptr;
    }

    win->EnsureSizeUpToDate();

    nsRefPtr<nsPresContext> presContext;
    docShell->GetPresContext(getter_AddRefs(presContext));
    if (presContext) {
      nsDeviceContext* context = presContext->DeviceContext();
      if (context) {
        return context;
      }
    }

    nsCOMPtr<nsIDocShellTreeItem> curItem = do_QueryInterface(docShell);
    nsCOMPtr<nsIDocShellTreeItem> parentItem;
    curItem->GetParent(getter_AddRefs(parentItem));
    docShell = do_QueryInterface(parentItem);
  }

  return nullptr;
}

 bool
nsLayoutUtils::IsReallyFixedPos(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame->GetParent(),
                  "IsReallyFixedPos called on frame not in tree");
  NS_PRECONDITION(aFrame->GetStyleDisplay()->mPosition ==
                    NS_STYLE_POSITION_FIXED,
                  "IsReallyFixedPos called on non-'position:fixed' frame");

  nsIAtom *parentType = aFrame->GetParent()->GetType();
  return parentType == nsGkAtoms::viewportFrame ||
         parentType == nsGkAtoms::pageContentFrame;
}

nsLayoutUtils::SurfaceFromElementResult
nsLayoutUtils::SurfaceFromElement(nsIImageLoadingContent* aElement,
                                  PRUint32 aSurfaceFlags)
{
  SurfaceFromElementResult result;
  nsresult rv;

  bool forceCopy = (aSurfaceFlags & SFE_WANT_NEW_SURFACE) != 0;
  bool wantImageSurface = (aSurfaceFlags & SFE_WANT_IMAGE_SURFACE) != 0;
  bool premultAlpha = (aSurfaceFlags & SFE_NO_PREMULTIPLY_ALPHA) == 0;

  if (!premultAlpha) {
    forceCopy = true;
    wantImageSurface = true;
  }

  
  
  
  nsCxPusher pusher;
  pusher.PushNull();

  nsCOMPtr<imgIRequest> imgRequest;
  rv = aElement->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                            getter_AddRefs(imgRequest));
  if (NS_FAILED(rv) || !imgRequest)
    return result;

  PRUint32 status;
  imgRequest->GetImageStatus(&status);
  if ((status & imgIRequest::STATUS_LOAD_COMPLETE) == 0) {
    
    
    
    result.mIsStillLoading = (status & imgIRequest::STATUS_ERROR) == 0;
    return result;
  }

  nsCOMPtr<nsIPrincipal> principal;
  rv = imgRequest->GetImagePrincipal(getter_AddRefs(principal));
  if (NS_FAILED(rv) || !principal)
    return result;

  nsCOMPtr<imgIContainer> imgContainer;
  rv = imgRequest->GetImage(getter_AddRefs(imgContainer));
  if (NS_FAILED(rv) || !imgContainer)
    return result;

  PRUint32 whichFrame = (aSurfaceFlags & SFE_WANT_FIRST_FRAME)
                        ? (PRUint32) imgIContainer::FRAME_FIRST
                        : (PRUint32) imgIContainer::FRAME_CURRENT;
  PRUint32 frameFlags = imgIContainer::FLAG_SYNC_DECODE;
  if (aSurfaceFlags & SFE_NO_COLORSPACE_CONVERSION)
    frameFlags |= imgIContainer::FLAG_DECODE_NO_COLORSPACE_CONVERSION;
  if (aSurfaceFlags & SFE_NO_PREMULTIPLY_ALPHA)
    frameFlags |= imgIContainer::FLAG_DECODE_NO_PREMULTIPLY_ALPHA;
  nsRefPtr<gfxASurface> framesurf;
  rv = imgContainer->GetFrame(whichFrame,
                              frameFlags,
                              getter_AddRefs(framesurf));
  if (NS_FAILED(rv))
    return result;

  PRInt32 imgWidth, imgHeight;
  rv = imgContainer->GetWidth(&imgWidth);
  nsresult rv2 = imgContainer->GetHeight(&imgHeight);
  if (NS_FAILED(rv) || NS_FAILED(rv2))
    return result;

  if (wantImageSurface && framesurf->GetType() != gfxASurface::SurfaceTypeImage) {
    forceCopy = true;
  }

  nsRefPtr<gfxASurface> gfxsurf = framesurf;
  if (forceCopy) {
    if (wantImageSurface) {
      gfxsurf = new gfxImageSurface (gfxIntSize(imgWidth, imgHeight), gfxASurface::ImageFormatARGB32);
    } else {
      gfxsurf = gfxPlatform::GetPlatform()->CreateOffscreenSurface(gfxIntSize(imgWidth, imgHeight),
                                                                   gfxASurface::CONTENT_COLOR_ALPHA);
    }

    nsRefPtr<gfxContext> ctx = new gfxContext(gfxsurf);

    ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    ctx->SetSource(framesurf);
    ctx->Paint();
  }

  PRInt32 corsmode;
  if (NS_SUCCEEDED(imgRequest->GetCORSMode(&corsmode))) {
    result.mCORSUsed = (corsmode != imgIRequest::CORS_NONE);
  }

  result.mSurface = gfxsurf;
  result.mSize = gfxIntSize(imgWidth, imgHeight);
  result.mPrincipal = principal.forget();
  
  result.mIsWriteOnly = false;
  result.mImageRequest = imgRequest.forget();

  return result;
}

nsLayoutUtils::SurfaceFromElementResult
nsLayoutUtils::SurfaceFromElement(nsHTMLImageElement *aElement,
                                  PRUint32 aSurfaceFlags)
{
  return SurfaceFromElement(static_cast<nsIImageLoadingContent*>(aElement),
                            aSurfaceFlags);
}

nsLayoutUtils::SurfaceFromElementResult
nsLayoutUtils::SurfaceFromElement(nsHTMLCanvasElement* aElement,
                                  PRUint32 aSurfaceFlags)
{
  SurfaceFromElementResult result;
  nsresult rv;

  bool forceCopy = (aSurfaceFlags & SFE_WANT_NEW_SURFACE) != 0;
  bool wantImageSurface = (aSurfaceFlags & SFE_WANT_IMAGE_SURFACE) != 0;
  bool premultAlpha = (aSurfaceFlags & SFE_NO_PREMULTIPLY_ALPHA) == 0;

  if (!premultAlpha) {
    forceCopy = true;
    wantImageSurface = true;
  }

  gfxIntSize size = aElement->GetSize();

  nsRefPtr<gfxASurface> surf;

  if (!forceCopy && aElement->CountContexts() == 1) {
    nsICanvasRenderingContextInternal *srcCanvas = aElement->GetContextAtIndex(0);
    rv = srcCanvas->GetThebesSurface(getter_AddRefs(surf));

    if (NS_FAILED(rv))
      surf = nullptr;
  }

  if (surf && wantImageSurface && surf->GetType() != gfxASurface::SurfaceTypeImage)
    surf = nullptr;

  if (!surf) {
    if (wantImageSurface) {
      surf = new gfxImageSurface(size, gfxASurface::ImageFormatARGB32);
    } else {
      surf = gfxPlatform::GetPlatform()->CreateOffscreenSurface(size, gfxASurface::CONTENT_COLOR_ALPHA);
    }

    nsRefPtr<gfxContext> ctx = new gfxContext(surf);
    
    PRUint32 flags = premultAlpha ? nsHTMLCanvasElement::RenderFlagPremultAlpha : 0;
    rv = aElement->RenderContextsExternal(ctx, gfxPattern::FILTER_NEAREST, flags);
    if (NS_FAILED(rv))
      return result;
  }

  
  
  aElement->MarkContextClean();

  result.mSurface = surf;
  result.mSize = size;
  result.mPrincipal = aElement->NodePrincipal();
  result.mIsWriteOnly = aElement->IsWriteOnly();

  return result;
}

nsLayoutUtils::SurfaceFromElementResult
nsLayoutUtils::SurfaceFromElement(nsHTMLVideoElement* aElement,
                                  PRUint32 aSurfaceFlags)
{
  SurfaceFromElementResult result;

  bool wantImageSurface = (aSurfaceFlags & SFE_WANT_IMAGE_SURFACE) != 0;
  bool premultAlpha = (aSurfaceFlags & SFE_NO_PREMULTIPLY_ALPHA) == 0;

  if (!premultAlpha) {
    wantImageSurface = true;
  }

  PRUint16 readyState;
  if (NS_SUCCEEDED(aElement->GetReadyState(&readyState)) &&
      (readyState == nsIDOMHTMLMediaElement::HAVE_NOTHING ||
       readyState == nsIDOMHTMLMediaElement::HAVE_METADATA)) {
    result.mIsStillLoading = true;
    return result;
  }

  
  nsCOMPtr<nsIPrincipal> principal = aElement->GetCurrentPrincipal();
  if (!principal)
    return result;

  ImageContainer *container = aElement->GetImageContainer();
  if (!container)
    return result;

  gfxIntSize size;
  nsRefPtr<gfxASurface> surf = container->GetCurrentAsSurface(&size);
  if (!surf)
    return result;

  if (wantImageSurface && surf->GetType() != gfxASurface::SurfaceTypeImage) {
    nsRefPtr<gfxImageSurface> imgSurf =
      new gfxImageSurface(size, gfxASurface::ImageFormatARGB32);

    nsRefPtr<gfxContext> ctx = new gfxContext(imgSurf);
    ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    ctx->DrawSurface(surf, size);
    surf = imgSurf;
  }

  result.mCORSUsed = aElement->GetCORSMode() != CORS_NONE;
  result.mSurface = surf;
  result.mSize = size;
  result.mPrincipal = principal.forget();
  result.mIsWriteOnly = false;

  return result;
}

nsLayoutUtils::SurfaceFromElementResult
nsLayoutUtils::SurfaceFromElement(dom::Element* aElement,
                                  PRUint32 aSurfaceFlags)
{
  
  if (nsHTMLCanvasElement* canvas = nsHTMLCanvasElement::FromContent(aElement)) {
    return SurfaceFromElement(canvas, aSurfaceFlags);
  }

#ifdef MOZ_MEDIA
  
  if (nsHTMLVideoElement* video = nsHTMLVideoElement::FromContent(aElement)) {
    return SurfaceFromElement(video, aSurfaceFlags);
  }
#endif

  
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(aElement);

  if (!imageLoader) {
    return SurfaceFromElementResult();
  }

  return SurfaceFromElement(imageLoader, aSurfaceFlags);
}


nsIContent*
nsLayoutUtils::GetEditableRootContentByContentEditable(nsIDocument* aDocument)
{
  
  if (!aDocument || aDocument->HasFlag(NODE_IS_EDITABLE)) {
    return nullptr;
  }

  
  
  
  
  nsCOMPtr<nsIDOMHTMLDocument> domHTMLDoc = do_QueryInterface(aDocument);
  if (!domHTMLDoc) {
    return nullptr;
  }

  Element* rootElement = aDocument->GetRootElement();
  if (rootElement && rootElement->IsEditable()) {
    return rootElement;
  }

  
  
  nsCOMPtr<nsIDOMHTMLElement> body;
  nsresult rv = domHTMLDoc->GetBody(getter_AddRefs(body));
  nsCOMPtr<nsIContent> content = do_QueryInterface(body);
  if (NS_SUCCEEDED(rv) && content && content->IsEditable()) {
    return content;
  }
  return nullptr;
}

#ifdef DEBUG
 void
nsLayoutUtils::AssertNoDuplicateContinuations(nsIFrame* aContainer,
                                              const nsFrameList& aFrameList)
{
  for (nsIFrame* f = aFrameList.FirstChild(); f ; f = f->GetNextSibling()) {
    
    
    
    for (nsIFrame *c = f; (c = c->GetNextInFlow());) {
      NS_ASSERTION(c->GetParent() != aContainer ||
                   !aFrameList.ContainsFrame(c),
                   "Two continuations of the same frame in the same "
                   "frame list");
    }
  }
}


static bool
IsInLetterFrame(nsIFrame *aFrame)
{
  for (nsIFrame *f = aFrame->GetParent(); f; f = f->GetParent()) {
    if (f->GetType() == nsGkAtoms::letterFrame) {
      return true;
    }
  }
  return false;
}

 void
nsLayoutUtils::AssertTreeOnlyEmptyNextInFlows(nsIFrame *aSubtreeRoot)
{
  NS_ASSERTION(aSubtreeRoot->GetPrevInFlow(),
               "frame tree not empty, but caller reported complete status");

  
  PRInt32 start, end;
  nsresult rv = aSubtreeRoot->GetOffsets(start, end);
  NS_ASSERTION(NS_SUCCEEDED(rv), "GetOffsets failed");
  
  
  
  
  
  
  
  NS_ASSERTION(start == end || IsInLetterFrame(aSubtreeRoot),
               "frame tree not empty, but caller reported complete status");

  nsIFrame::ChildListIterator lists(aSubtreeRoot);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsLayoutUtils::AssertTreeOnlyEmptyNextInFlows(childFrames.get());
    }
  }
}
#endif


nsresult
nsLayoutUtils::GetFontFacesForFrames(nsIFrame* aFrame,
                                     nsFontFaceList* aFontFaceList)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");

  if (aFrame->GetType() == nsGkAtoms::textFrame) {
    return GetFontFacesForText(aFrame, 0, PR_INT32_MAX, false,
                               aFontFaceList);
  }

  while (aFrame) {
    nsIFrame::ChildListID childLists[] = { nsIFrame::kPrincipalList,
                                           nsIFrame::kPopupList };
    for (size_t i = 0; i < ArrayLength(childLists); ++i) {
      nsFrameList children(aFrame->GetChildList(childLists[i]));
      for (nsFrameList::Enumerator e(children); !e.AtEnd(); e.Next()) {
        nsIFrame* child = e.get();
        if (child->GetPrevContinuation()) {
          continue;
        }
        child = nsPlaceholderFrame::GetRealFrameFor(child);
        nsresult rv = GetFontFacesForFrames(child, aFontFaceList);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    aFrame = GetNextContinuationOrSpecialSibling(aFrame);
  }

  return NS_OK;
}


nsresult
nsLayoutUtils::GetFontFacesForText(nsIFrame* aFrame,
                                   PRInt32 aStartOffset, PRInt32 aEndOffset,
                                   bool aFollowContinuations,
                                   nsFontFaceList* aFontFaceList)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");

  if (aFrame->GetType() != nsGkAtoms::textFrame) {
    return NS_OK;
  }

  nsTextFrame* curr = static_cast<nsTextFrame*>(aFrame);
  do {
    PRInt32 fstart = NS_MAX(curr->GetContentOffset(), aStartOffset);
    PRInt32 fend = NS_MIN(curr->GetContentEnd(), aEndOffset);
    if (fstart >= fend) {
      continue;
    }

    
    gfxSkipCharsIterator iter = curr->EnsureTextRun(nsTextFrame::eInflated);
    gfxTextRun* textRun = curr->GetTextRun(nsTextFrame::eInflated);
    NS_ENSURE_TRUE(textRun, NS_ERROR_OUT_OF_MEMORY);

    PRUint32 skipStart = iter.ConvertOriginalToSkipped(fstart);
    PRUint32 skipEnd = iter.ConvertOriginalToSkipped(fend);
    aFontFaceList->AddFontsFromTextRun(textRun,
                                       skipStart,
                                       skipEnd - skipStart,
                                       curr);
  } while (aFollowContinuations &&
           (curr = static_cast<nsTextFrame*>(curr->GetNextContinuation())));

  return NS_OK;
}


size_t
nsLayoutUtils::SizeOfTextRunsForFrames(nsIFrame* aFrame,
                                       nsMallocSizeOfFun aMallocSizeOf,
                                       bool clear)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");

  size_t total = 0;

  if (aFrame->GetType() == nsGkAtoms::textFrame) {
    nsTextFrame* textFrame = static_cast<nsTextFrame*>(aFrame);
    for (PRUint32 i = 0; i < 2; ++i) {
      gfxTextRun *run = textFrame->GetTextRun(
        (i != 0) ? nsTextFrame::eInflated : nsTextFrame::eNotInflated);
      if (run) {
        if (clear) {
          run->ResetSizeOfAccountingFlags();
        } else {
          total += run->MaybeSizeOfIncludingThis(aMallocSizeOf);
        }
      }
    }
    return total;
  }

  nsAutoTArray<nsIFrame::ChildList,4> childListArray;
  aFrame->GetChildLists(&childListArray);

  for (nsIFrame::ChildListArrayIterator childLists(childListArray);
       !childLists.IsDone(); childLists.Next()) {
    for (nsFrameList::Enumerator e(childLists.CurrentList());
         !e.AtEnd(); e.Next()) {
      total += SizeOfTextRunsForFrames(e.get(), aMallocSizeOf, clear);
    }
  }
  return total;
}


void
nsLayoutUtils::Initialize()
{
  mozilla::Preferences::AddUintVarCache(&sFontSizeInflationEmPerLine,
                                        "font.size.inflation.emPerLine");
  mozilla::Preferences::AddUintVarCache(&sFontSizeInflationMinTwips,
                                        "font.size.inflation.minTwips");
  mozilla::Preferences::AddUintVarCache(&sFontSizeInflationLineThreshold,
                                        "font.size.inflation.lineThreshold");
}


void
nsLayoutUtils::Shutdown()
{
  if (sContentMap) {
    delete sContentMap;
    sContentMap = NULL;
  }
}


void
nsLayoutUtils::RegisterImageRequest(nsPresContext* aPresContext,
                                    imgIRequest* aRequest,
                                    bool* aRequestRegistered)
{
  if (!aPresContext) {
    return;
  }

  if (aRequestRegistered && *aRequestRegistered) {
    
    
    return;
  }

  if (aRequest) {
    if (!aPresContext->RefreshDriver()->AddImageRequest(aRequest)) {
      NS_WARNING("Unable to add image request");
      return;
    }

    if (aRequestRegistered) {
      *aRequestRegistered = true;
    }
  }
}


void
nsLayoutUtils::RegisterImageRequestIfAnimated(nsPresContext* aPresContext,
                                              imgIRequest* aRequest,
                                              bool* aRequestRegistered)
{
  if (!aPresContext) {
    return;
  }

  if (aRequestRegistered && *aRequestRegistered) {
    
    
    return;
  }

  if (aRequest) {
    nsCOMPtr<imgIContainer> image;
    aRequest->GetImage(getter_AddRefs(image));
    if (image) {

      
      
      bool isAnimated = false;
      nsresult rv = image->GetAnimated(&isAnimated);
      if (NS_SUCCEEDED(rv) && isAnimated) {
        if (!aPresContext->RefreshDriver()->AddImageRequest(aRequest)) {
          NS_WARNING("Unable to add image request");
          return;
        }

        if (aRequestRegistered) {
          *aRequestRegistered = true;
        }
      }
    }
  }
}


void
nsLayoutUtils::DeregisterImageRequest(nsPresContext* aPresContext,
                                      imgIRequest* aRequest,
                                      bool* aRequestRegistered)
{
  if (!aPresContext) {
    return;
  }

  
  
  if (aRequestRegistered && !*aRequestRegistered) {
    return;
  }

  if (aRequest) {
    nsCOMPtr<imgIContainer> image;
    aRequest->GetImage(getter_AddRefs(image));
    if (image) {
      aPresContext->RefreshDriver()->RemoveImageRequest(aRequest);

      if (aRequestRegistered) {
        *aRequestRegistered = false;
      }
    }
  }
}


void
nsLayoutUtils::PostRestyleEvent(Element* aElement,
                                nsRestyleHint aRestyleHint,
                                nsChangeHint aMinChangeHint)
{
  nsIDocument* doc = aElement->GetCurrentDoc();
  if (doc) {
    nsCOMPtr<nsIPresShell> presShell = doc->GetShell();
    if (presShell) {
      presShell->FrameConstructor()->PostRestyleEvent(
        aElement, aRestyleHint, aMinChangeHint);
    }
  }
}

nsSetAttrRunnable::nsSetAttrRunnable(nsIContent* aContent, nsIAtom* aAttrName,
                                     const nsAString& aValue)
  : mContent(aContent),
    mAttrName(aAttrName),
    mValue(aValue)
{
  NS_ASSERTION(aContent && aAttrName, "Missing stuff, prepare to crash");
}

nsSetAttrRunnable::nsSetAttrRunnable(nsIContent* aContent, nsIAtom* aAttrName,
                                     PRInt32 aValue)
  : mContent(aContent),
    mAttrName(aAttrName)
{
  NS_ASSERTION(aContent && aAttrName, "Missing stuff, prepare to crash");
  mValue.AppendInt(aValue);
}

NS_IMETHODIMP
nsSetAttrRunnable::Run()
{
  return mContent->SetAttr(kNameSpaceID_None, mAttrName, mValue, true);
}

nsUnsetAttrRunnable::nsUnsetAttrRunnable(nsIContent* aContent,
                                         nsIAtom* aAttrName)
  : mContent(aContent),
    mAttrName(aAttrName)
{
  NS_ASSERTION(aContent && aAttrName, "Missing stuff, prepare to crash");
}

NS_IMETHODIMP
nsUnsetAttrRunnable::Run()
{
  return mContent->UnsetAttr(kNameSpaceID_None, mAttrName, true);
}

nsReflowFrameRunnable::nsReflowFrameRunnable(nsIFrame* aFrame,
                          nsIPresShell::IntrinsicDirty aIntrinsicDirty,
                          nsFrameState aBitToAdd)
  : mWeakFrame(aFrame),
    mIntrinsicDirty(aIntrinsicDirty),
    mBitToAdd(aBitToAdd)
{
}

NS_IMETHODIMP
nsReflowFrameRunnable::Run()
{
  if (mWeakFrame.IsAlive()) {
    mWeakFrame->PresContext()->PresShell()->
      FrameNeedsReflow(mWeakFrame, mIntrinsicDirty, mBitToAdd);
  }
  return NS_OK;
}






static nscoord
MinimumFontSizeFor(nsPresContext* aPresContext, nscoord aContainerWidth)
{
  nsIPresShell* presShell = aPresContext->PresShell();

  PRUint32 emPerLine = presShell->FontSizeInflationEmPerLine();
  PRUint32 minTwips = presShell->FontSizeInflationMinTwips();
  if (emPerLine == 0 && minTwips == 0) {
    return 0;
  }

  
  nscoord iFrameWidth = aPresContext->GetVisibleArea().width;
  nscoord effectiveContainerWidth = NS_MIN(iFrameWidth, aContainerWidth);

  nscoord byLine = 0, byInch = 0;
  if (emPerLine != 0) {
    byLine = effectiveContainerWidth / emPerLine;
  }
  if (minTwips != 0) {
    
    
    float deviceWidthInches =
      aPresContext->ScreenWidthInchesForFontInflation();
    byInch = NSToCoordRound(effectiveContainerWidth /
                            (deviceWidthInches * 1440 /
                             minTwips ));
  }
  return NS_MAX(byLine, byInch);
}

 float
nsLayoutUtils::FontSizeInflationInner(const nsIFrame *aFrame,
                                      nscoord aMinFontSize)
{
  
  
  
  nscoord styleFontSize = aFrame->GetStyleFont()->mFont.size;
  if (styleFontSize <= 0) {
    
    return 1.0;
  }

  if (aMinFontSize <= 0) {
    
    return 1.0;
  }

  
  
  
  for (const nsIFrame* f = aFrame;
       f && !IsContainerForFontSizeInflation(f);
       f = f->GetParent()) {
    nsIContent* content = f->GetContent();
    nsIAtom* fType = f->GetType();
    
    
    if (!(f->GetParent() && f->GetParent()->GetContent() == content) &&
        
        fType != nsGkAtoms::inlineFrame &&
        
        
        fType != nsGkAtoms::formControlFrame) {
      nsStyleCoord stylePosWidth = f->GetStylePosition()->mWidth;
      nsStyleCoord stylePosHeight = f->GetStylePosition()->mHeight;
      if (stylePosWidth.GetUnit() != eStyleUnit_Auto ||
          stylePosHeight.GetUnit() != eStyleUnit_Auto) {

        return 1.0;
      }
    }
  }

  
  
  
  
  float ratio = float(styleFontSize) / float(aMinFontSize);
  if (ratio >= 1.5f) {
    
    return 1.0;
  }

  
  
  
  
  return (1.0f / ratio) + (1.0f / 3.0f);
}

static bool
ShouldInflateFontsForContainer(const nsIFrame *aFrame)
{
  
  
  
  
  
  
  
  const nsStyleText* styleText = aFrame->GetStyleText();

  return styleText->mTextSizeAdjust != NS_STYLE_TEXT_SIZE_ADJUST_NONE &&
         !(aFrame->GetStateBits() & NS_FRAME_IN_CONSTRAINED_HEIGHT) &&
         
         
         styleText->WhiteSpaceCanWrap();
}

nscoord
nsLayoutUtils::InflationMinFontSizeFor(const nsIFrame *aFrame)
{
  nsPresContext *presContext = aFrame->PresContext();
  if (!FontSizeInflationEnabled(presContext) ||
      presContext->mInflationDisabledForShrinkWrap) {
    return 0;
  }

  for (const nsIFrame *f = aFrame; f; f = f->GetParent()) {
    if (IsContainerForFontSizeInflation(f)) {
      if (!ShouldInflateFontsForContainer(f)) {
        return 0;
      }

      nsFontInflationData *data =
        nsFontInflationData::FindFontInflationDataFor(aFrame);
      
      
      if (!data || !data->InflationEnabled()) {
        return 0;
      }

      return MinimumFontSizeFor(aFrame->PresContext(),
                                data->EffectiveWidth());
    }
  }

  NS_ABORT_IF_FALSE(false, "root should always be container");

  return 0;
}

float
nsLayoutUtils::FontSizeInflationFor(const nsIFrame *aFrame)
{
  if (!FontSizeInflationEnabled(aFrame->PresContext())) {
    return 1.0;
  }

  return FontSizeInflationInner(aFrame, InflationMinFontSizeFor(aFrame));
}

 bool
nsLayoutUtils::FontSizeInflationEnabled(nsPresContext *aPresContext)
{
  nsIPresShell* presShell = aPresContext->GetPresShell();

  if (!presShell ||
      (presShell->FontSizeInflationEmPerLine() == 0 &&
       presShell->FontSizeInflationMinTwips() == 0) ||
       aPresContext->IsChrome()) {
    return false;
  }

  ViewportInfo vInf =
    nsContentUtils::GetViewportInfo(aPresContext->PresShell()->GetDocument());

  if (vInf.defaultZoom >= 1.0 || vInf.autoSize) {
    return false;
  }

  return true;
}
