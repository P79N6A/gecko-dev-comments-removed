



#include "PositionedEventTargeting.h"

#include "mozilla/EventListenerManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Preferences.h"
#include "nsLayoutUtils.h"
#include "nsGkAtoms.h"
#include "nsFontMetrics.h"
#include "nsPrintfCString.h"
#include "mozilla/dom/Element.h"
#include "nsRegion.h"
#include "nsDeviceContext.h"
#include "nsIFrame.h"
#include <algorithm>
#include "LayersLogging.h"



#define PET_LOG(...)


namespace mozilla {











































struct EventRadiusPrefs
{
  uint32_t mVisitedWeight; 
  uint32_t mSideRadii[4]; 
  bool mEnabled;
  bool mRegistered;
  bool mTouchOnly;
  bool mRepositionEventCoords;
  bool mTouchClusterDetection;
  uint32_t mLimitReadableSize;
};

static EventRadiusPrefs sMouseEventRadiusPrefs;
static EventRadiusPrefs sTouchEventRadiusPrefs;

static const EventRadiusPrefs*
GetPrefsFor(EventClassID aEventClassID)
{
  EventRadiusPrefs* prefs = nullptr;
  const char* prefBranch = nullptr;
  if (aEventClassID == eTouchEventClass) {
    prefBranch = "touch";
    prefs = &sTouchEventRadiusPrefs;
  } else if (aEventClassID == eMouseEventClass) {
    
    prefBranch = "mouse";
    prefs = &sMouseEventRadiusPrefs;
  } else {
    return nullptr;
  }

  if (!prefs->mRegistered) {
    prefs->mRegistered = true;

    nsPrintfCString enabledPref("ui.%s.radius.enabled", prefBranch);
    Preferences::AddBoolVarCache(&prefs->mEnabled, enabledPref.get(), false);

    nsPrintfCString visitedWeightPref("ui.%s.radius.visitedWeight", prefBranch);
    Preferences::AddUintVarCache(&prefs->mVisitedWeight, visitedWeightPref.get(), 100);

    static const char prefNames[4][9] =
      { "topmm", "rightmm", "bottommm", "leftmm" };
    for (int32_t i = 0; i < 4; ++i) {
      nsPrintfCString radiusPref("ui.%s.radius.%s", prefBranch, prefNames[i]);
      Preferences::AddUintVarCache(&prefs->mSideRadii[i], radiusPref.get(), 0);
    }

    if (aEventClassID == eMouseEventClass) {
      Preferences::AddBoolVarCache(&prefs->mTouchOnly,
          "ui.mouse.radius.inputSource.touchOnly", true);
    } else {
      prefs->mTouchOnly = false;
    }

    nsPrintfCString repositionPref("ui.%s.radius.reposition", prefBranch);
    Preferences::AddBoolVarCache(&prefs->mRepositionEventCoords, repositionPref.get(), false);

    nsPrintfCString touchClusterPref("ui.zoomedview.enabled", prefBranch);
    Preferences::AddBoolVarCache(&prefs->mTouchClusterDetection, touchClusterPref.get(), false);

    nsPrintfCString limitReadableSizePref("ui.zoomedview.limitReadableSize", prefBranch);
    Preferences::AddUintVarCache(&prefs->mLimitReadableSize, limitReadableSizePref.get(), 8);
  }

  return prefs;
}

static bool
HasMouseListener(nsIContent* aContent)
{
  if (EventListenerManager* elm = aContent->GetExistingListenerManager()) {
    return elm->HasListenersFor(nsGkAtoms::onclick) ||
           elm->HasListenersFor(nsGkAtoms::onmousedown) ||
           elm->HasListenersFor(nsGkAtoms::onmouseup);
  }

  return false;
}

static bool gTouchEventsRegistered = false;
static int32_t gTouchEventsEnabled = 0;

static bool
HasTouchListener(nsIContent* aContent)
{
  EventListenerManager* elm = aContent->GetExistingListenerManager();
  if (!elm) {
    return false;
  }

  if (!gTouchEventsRegistered) {
    Preferences::AddIntVarCache(&gTouchEventsEnabled,
      "dom.w3c_touch_events.enabled", gTouchEventsEnabled);
    gTouchEventsRegistered = true;
  }

  if (!gTouchEventsEnabled) {
    return false;
  }

  return elm->HasListenersFor(nsGkAtoms::ontouchstart) ||
         elm->HasListenersFor(nsGkAtoms::ontouchend);
}

static bool
IsElementClickable(nsIFrame* aFrame, nsIAtom* stopAt = nullptr)
{
  
  
  for (nsIContent* content = aFrame->GetContent(); content;
       content = content->GetFlattenedTreeParent()) {
    nsIAtom* tag = content->Tag();
    if (content->IsHTML() && stopAt && tag == stopAt) {
      break;
    }
    if (HasTouchListener(content) || HasMouseListener(content)) {
      return true;
    }
    if (content->IsHTML()) {
      if (tag == nsGkAtoms::button ||
          tag == nsGkAtoms::input ||
          tag == nsGkAtoms::select ||
          tag == nsGkAtoms::textarea ||
          tag == nsGkAtoms::label) {
        return true;
      }
      
      
      
      if (tag == nsGkAtoms::iframe &&
          content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::mozbrowser,
                               nsGkAtoms::_true, eIgnoreCase) &&
          content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::Remote,
                               nsGkAtoms::_true, eIgnoreCase)) {
        return true;
      }
    } else if (content->IsXUL()) {
      nsIAtom* tag = content->Tag();
      
      
      if (tag == nsGkAtoms::button ||
          tag == nsGkAtoms::checkbox ||
          tag == nsGkAtoms::radio ||
          tag == nsGkAtoms::autorepeatbutton ||
          tag == nsGkAtoms::menu ||
          tag == nsGkAtoms::menubutton ||
          tag == nsGkAtoms::menuitem ||
          tag == nsGkAtoms::menulist ||
          tag == nsGkAtoms::scrollbarbutton ||
          tag == nsGkAtoms::resizer) {
        return true;
      }
    }
    static nsIContent::AttrValuesArray clickableRoles[] =
      { &nsGkAtoms::button, &nsGkAtoms::key, nullptr };
    if (content->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::role,
                                 clickableRoles, eIgnoreCase) >= 0) {
      return true;
    }
    if (content->IsEditable()) {
      return true;
    }
    nsCOMPtr<nsIURI> linkURI;
    if (content->IsLink(getter_AddRefs(linkURI))) {
      return true;
    }
  }
  return false;
}

static nscoord
AppUnitsFromMM(nsIFrame* aFrame, uint32_t aMM, bool aVertical)
{
  nsPresContext* pc = aFrame->PresContext();
  float result = float(aMM) *
    (pc->DeviceContext()->AppUnitsPerPhysicalInch() / MM_PER_INCH_FLOAT);
  return NSToCoordRound(result);
}





static nsRect
ClipToFrame(nsIFrame* aRootFrame, nsIFrame* aFrame, nsRect& aRect)
{
  nsRect bound = nsLayoutUtils::TransformFrameRectToAncestor(
    aFrame, nsRect(nsPoint(0, 0), aFrame->GetSize()), aRootFrame);
  nsRect result = bound.Intersect(aRect);
  return result;
}

static nsRect
GetTargetRect(nsIFrame* aRootFrame, const nsPoint& aPointRelativeToRootFrame,
              nsIFrame* aRestrictToDescendants, const EventRadiusPrefs* aPrefs,
              uint32_t aFlags)
{
  nsMargin m(AppUnitsFromMM(aRootFrame, aPrefs->mSideRadii[0], true),
             AppUnitsFromMM(aRootFrame, aPrefs->mSideRadii[1], false),
             AppUnitsFromMM(aRootFrame, aPrefs->mSideRadii[2], true),
             AppUnitsFromMM(aRootFrame, aPrefs->mSideRadii[3], false));
  nsRect r(aPointRelativeToRootFrame, nsSize(0,0));
  r.Inflate(m);
  if (!(aFlags & INPUT_IGNORE_ROOT_SCROLL_FRAME)) {
    
    
    
    r = ClipToFrame(aRootFrame, aRestrictToDescendants, r);
  }
  return r;
}

static float
ComputeDistanceFromRect(const nsPoint& aPoint, const nsRect& aRect)
{
  nscoord dx = std::max(0, std::max(aRect.x - aPoint.x, aPoint.x - aRect.XMost()));
  nscoord dy = std::max(0, std::max(aRect.y - aPoint.y, aPoint.y - aRect.YMost()));
  return float(NS_hypot(dx, dy));
}

static float
ComputeDistanceFromRegion(const nsPoint& aPoint, const nsRegion& aRegion)
{
  MOZ_ASSERT(!aRegion.IsEmpty(), "can't compute distance between point and empty region");
  nsRegionRectIterator iter(aRegion);
  const nsRect* r;
  float minDist = -1;
  while ((r = iter.Next()) != nullptr) {
    float dist = ComputeDistanceFromRect(aPoint, *r);
    if (dist < minDist || minDist < 0) {
      minDist = dist;
    }
  }
  return minDist;
}



static void
SubtractFromExposedRegion(nsRegion* aExposedRegion, const nsRegion& aRegion)
{
  if (aRegion.IsEmpty())
    return;

  nsRegion tmp;
  tmp.Sub(*aExposedRegion, aRegion);
  
  
  
  if (tmp.GetNumRects() <= 15 || tmp.Area() <= aExposedRegion->Area()/2) {
    *aExposedRegion = tmp;
  }
}

static nsIFrame*
GetClosest(nsIFrame* aRoot, const nsPoint& aPointRelativeToRootFrame,
           const nsRect& aTargetRect, const EventRadiusPrefs* aPrefs,
           nsIFrame* aRestrictToDescendants, nsTArray<nsIFrame*>& aCandidates,
           int32_t* aElementsInCluster)
{
  nsIFrame* bestTarget = nullptr;
  
  float bestDistance = 1e6f;
  nsRegion exposedRegion(aTargetRect);
  for (uint32_t i = 0; i < aCandidates.Length(); ++i) {
    nsIFrame* f = aCandidates[i];
    PET_LOG("Checking candidate %p\n", f);

    bool preservesAxisAlignedRectangles = false;
    nsRect borderBox = nsLayoutUtils::TransformFrameRectToAncestor(f,
        nsRect(nsPoint(0, 0), f->GetSize()), aRoot, &preservesAxisAlignedRectangles);
    nsRegion region;
    region.And(exposedRegion, borderBox);

    if (region.IsEmpty()) {
      PET_LOG("  candidate %p had empty hit region\n", f);
      continue;
    }

    if (preservesAxisAlignedRectangles) {
      
      
      SubtractFromExposedRegion(&exposedRegion, region);
    }

    if (!IsElementClickable(f, nsGkAtoms::body)) {
      PET_LOG("  candidate %p was not clickable\n", f);
      continue;
    }
    
    
    if (bestTarget && nsLayoutUtils::IsProperAncestorFrameCrossDoc(f, bestTarget, aRoot)) {
      PET_LOG("  candidate %p was ancestor for bestTarget %p\n", f, bestTarget);
      continue;
    }
    if (!nsLayoutUtils::IsAncestorFrameCrossDoc(aRestrictToDescendants, f, aRoot)) {
      PET_LOG("  candidate %p was not descendant of restrictroot %p\n", f, aRestrictToDescendants);
      continue;
    }

    (*aElementsInCluster)++;

    
    float distance = ComputeDistanceFromRegion(aPointRelativeToRootFrame, region);
    nsIContent* content = f->GetContent();
    if (content && content->IsElement() &&
        content->AsElement()->State().HasState(
                                        EventStates(NS_EVENT_STATE_VISITED))) {
      distance *= aPrefs->mVisitedWeight / 100.0f;
    }
    if (distance < bestDistance) {
      PET_LOG("  candidate %p is the new best\n", f);
      bestDistance = distance;
      bestTarget = f;
    }
  }
  return bestTarget;
}









static bool
IsElementClickableAndReadable(nsIFrame* aFrame, WidgetGUIEvent* aEvent, const EventRadiusPrefs* aPrefs)
{
  if (!aPrefs->mTouchClusterDetection) {
    return true;
  }

  if (aEvent->mClass != eMouseEventClass) {
    return true;
  }

  uint32_t limitReadableSize = aPrefs->mLimitReadableSize;
  nsSize frameSize = aFrame->GetSize();
  nsPresContext* pc = aFrame->PresContext();
  nsIPresShell* presShell = pc->PresShell();
  gfxSize cumulativeResolution = presShell->GetCumulativeResolution();
  if ((pc->AppUnitsToGfxUnits(frameSize.height) * cumulativeResolution.height) < limitReadableSize ||
      (pc->AppUnitsToGfxUnits(frameSize.width) * cumulativeResolution.width) < limitReadableSize) {
    return false;
  }
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(fm),
    nsLayoutUtils::FontSizeInflationFor(aFrame));
  if (fm) {
    if ((pc->AppUnitsToGfxUnits(fm->EmHeight()) * cumulativeResolution.height) < limitReadableSize) {
      return false;
    }
  }

  return true;
}

nsIFrame*
FindFrameTargetedByInputEvent(WidgetGUIEvent* aEvent,
                              nsIFrame* aRootFrame,
                              const nsPoint& aPointRelativeToRootFrame,
                              uint32_t aFlags)
{
  uint32_t flags = (aFlags & INPUT_IGNORE_ROOT_SCROLL_FRAME) ?
     nsLayoutUtils::IGNORE_ROOT_SCROLL_FRAME : 0;
  nsIFrame* target =
    nsLayoutUtils::GetFrameForPoint(aRootFrame, aPointRelativeToRootFrame, flags);
  PET_LOG("Found initial target %p for event class %s point %s relative to root frame %p\n",
    target, (aEvent->mClass == eMouseEventClass ? "mouse" :
             (aEvent->mClass == eTouchEventClass ? "touch" : "other")),
    mozilla::layers::Stringify(aPointRelativeToRootFrame).c_str(), aRootFrame);

  const EventRadiusPrefs* prefs = GetPrefsFor(aEvent->mClass);
  if (!prefs || !prefs->mEnabled) {
    PET_LOG("Retargeting disabled\n");
    return target;
  }
  if (target && IsElementClickable(target, nsGkAtoms::body)) {
    if (!IsElementClickableAndReadable(target, aEvent, prefs)) {
      aEvent->AsMouseEventBase()->hitCluster = true;
    }
    PET_LOG("Target %p is clickable\n", target);
    return target;
  }

  
  
  if (aEvent->mClass == eMouseEventClass &&
      prefs->mTouchOnly &&
      aEvent->AsMouseEvent()->inputSource !=
        nsIDOMMouseEvent::MOZ_SOURCE_TOUCH) {
    PET_LOG("Mouse input event is not from a touch source\n");
    return target;
  }

  
  
  
  
  
  nsIFrame* restrictToDescendants = target ?
    target->PresContext()->PresShell()->GetRootFrame() : aRootFrame;

  nsRect targetRect = GetTargetRect(aRootFrame, aPointRelativeToRootFrame,
                                    restrictToDescendants, prefs, aFlags);
  PET_LOG("Expanded point to target rect %s\n",
    mozilla::layers::Stringify(targetRect).c_str());
  nsAutoTArray<nsIFrame*,8> candidates;
  nsresult rv = nsLayoutUtils::GetFramesForArea(aRootFrame, targetRect, candidates, flags);
  if (NS_FAILED(rv)) {
    return target;
  }

  int32_t elementsInCluster = 0;

  nsIFrame* closestClickable =
    GetClosest(aRootFrame, aPointRelativeToRootFrame, targetRect, prefs,
               restrictToDescendants, candidates, &elementsInCluster);
  if (closestClickable) {
    if ((prefs->mTouchClusterDetection && elementsInCluster > 1) ||
        (!IsElementClickableAndReadable(closestClickable, aEvent, prefs))) {
      if (aEvent->mClass == eMouseEventClass) {
        WidgetMouseEventBase* mouseEventBase = aEvent->AsMouseEventBase();
        mouseEventBase->hitCluster = true;
      }
    }
    target = closestClickable;
  }
  PET_LOG("Final target is %p\n", target);

  
  
  
  

  if (!target || !prefs->mRepositionEventCoords) {
    
    return target;
  }

  
  
  nsPoint point = aPointRelativeToRootFrame;
  if (nsLayoutUtils::TRANSFORM_SUCCEEDED != nsLayoutUtils::TransformPoint(aRootFrame, target, point)) {
    return target;
  }
  point = target->GetRectRelativeToSelf().ClampPoint(point);
  if (nsLayoutUtils::TRANSFORM_SUCCEEDED != nsLayoutUtils::TransformPoint(target, aRootFrame, point)) {
    return target;
  }
  
  
  nsView* view = aRootFrame->GetView();
  if (!view) {
    return target;
  }
  LayoutDeviceIntPoint widgetPoint = nsLayoutUtils::TranslateViewToWidget(
        aRootFrame->PresContext(), view, point, aEvent->widget);
  if (widgetPoint.x != NS_UNCONSTRAINEDSIZE) {
    
    aEvent->refPoint = widgetPoint;
  }
  return target;
}

}
