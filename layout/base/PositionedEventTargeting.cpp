



#include "PositionedEventTargeting.h"

#include "mozilla/Preferences.h"
#include "nsGUIEvent.h"
#include "nsLayoutUtils.h"
#include "nsGkAtoms.h"
#include "nsEventListenerManager.h"
#include "nsPrintfCString.h"
#include "mozilla/dom/Element.h"
#include <algorithm>

namespace mozilla {











































struct EventRadiusPrefs
{
  uint32_t mVisitedWeight; 
  uint32_t mSideRadii[4]; 
  bool mEnabled;
  bool mRegistered;
  bool mTouchOnly;
};

static EventRadiusPrefs sMouseEventRadiusPrefs;
static EventRadiusPrefs sTouchEventRadiusPrefs;

static const EventRadiusPrefs*
GetPrefsFor(nsEventStructType aEventStructType)
{
  EventRadiusPrefs* prefs = nullptr;
  const char* prefBranch = nullptr;
  if (aEventStructType == NS_TOUCH_EVENT) {
    prefBranch = "touch";
    prefs = &sTouchEventRadiusPrefs;
  } else if (aEventStructType == NS_MOUSE_EVENT) {
    
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

    if (aEventStructType == NS_MOUSE_EVENT) {
      Preferences::AddBoolVarCache(&prefs->mTouchOnly,
          "ui.mouse.radius.inputSource.touchOnly", true);
    } else {
      prefs->mTouchOnly = false;
    }
  }

  return prefs;
}

static bool
HasMouseListener(nsIContent* aContent)
{
  nsEventListenerManager* elm = aContent->GetListenerManager(false);
  if (!elm) {
    return false;
  }
  return elm->HasListenersFor(nsGkAtoms::onclick) ||
         elm->HasListenersFor(nsGkAtoms::onmousedown) ||
         elm->HasListenersFor(nsGkAtoms::onmouseup);
}

static bool
IsElementClickable(nsIFrame* aFrame)
{
  
  
  for (nsIContent* content = aFrame->GetContent(); content;
       content = content->GetFlattenedTreeParent()) {
    if (HasMouseListener(content)) {
      return true;
    }
    if (content->IsHTML()) {
      nsIAtom* tag = content->Tag();
      if (tag == nsGkAtoms::button ||
          tag == nsGkAtoms::input ||
          tag == nsGkAtoms::select ||
          tag == nsGkAtoms::textarea ||
          tag == nsGkAtoms::label) {
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
    if (content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::role,
                             nsGkAtoms::button, eIgnoreCase)) {
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
  nsIPresShell* presShell = pc->PresShell();
  float result = float(aMM) *
    (pc->DeviceContext()->AppUnitsPerPhysicalInch() / MM_PER_INCH_FLOAT) *
    (aVertical ? presShell->GetYResolution() : presShell->GetXResolution());
  return NSToCoordRound(result);
}

static nsRect
GetTargetRect(nsIFrame* aRootFrame, const nsPoint& aPointRelativeToRootFrame,
              const EventRadiusPrefs* aPrefs)
{
  nsMargin m(AppUnitsFromMM(aRootFrame, aPrefs->mSideRadii[0], true),
             AppUnitsFromMM(aRootFrame, aPrefs->mSideRadii[1], false),
             AppUnitsFromMM(aRootFrame, aPrefs->mSideRadii[2], true),
             AppUnitsFromMM(aRootFrame, aPrefs->mSideRadii[3], false));
  nsRect r(aPointRelativeToRootFrame, nsSize(0,0));
  r.Inflate(m);
  return r;
}

static float
ComputeDistanceFromRect(const nsPoint& aPoint, const nsRect& aRect)
{
  nscoord dx = std::max(0, std::max(aRect.x - aPoint.x, aPoint.x - aRect.XMost()));
  nscoord dy = std::max(0, std::max(aRect.y - aPoint.y, aPoint.y - aRect.YMost()));
  return float(NS_hypot(dx, dy));
}

static nsIFrame*
GetClosest(nsIFrame* aRoot, const nsPoint& aPointRelativeToRootFrame,
           const EventRadiusPrefs* aPrefs, nsIFrame* aRestrictToDescendants,
           nsTArray<nsIFrame*>& aCandidates)
{
  nsIFrame* bestTarget = nullptr;
  
  float bestDistance = 1e6f;
  for (uint32_t i = 0; i < aCandidates.Length(); ++i) {
    nsIFrame* f = aCandidates[i];
    if (!IsElementClickable(f)) {
      continue;
    }
    
    
    if (bestTarget && nsLayoutUtils::IsProperAncestorFrameCrossDoc(f, bestTarget, aRoot)) {
      continue;
    }
    if (!nsLayoutUtils::IsAncestorFrameCrossDoc(aRestrictToDescendants, f, aRoot)) {
      continue;
    }

    nsRect borderBox = nsLayoutUtils::TransformFrameRectToAncestor(f,
        nsRect(nsPoint(0, 0), f->GetSize()), aRoot);
    
    float distance = ComputeDistanceFromRect(aPointRelativeToRootFrame, borderBox);
    nsIContent* content = f->GetContent();
    if (content && content->IsElement() &&
        content->AsElement()->State().HasState(nsEventStates(NS_EVENT_STATE_VISITED))) {
      distance *= aPrefs->mVisitedWeight / 100.0f;
    }
    if (distance < bestDistance) {
      bestDistance = distance;
      bestTarget = f;
    }
  }
  return bestTarget;
}

nsIFrame*
FindFrameTargetedByInputEvent(const nsGUIEvent *aEvent,
                              nsIFrame* aRootFrame,
                              const nsPoint& aPointRelativeToRootFrame,
                              uint32_t aFlags)
{
  bool ignoreRootScrollFrame = (aFlags & INPUT_IGNORE_ROOT_SCROLL_FRAME) != 0;
  nsIFrame* target =
    nsLayoutUtils::GetFrameForPoint(aRootFrame, aPointRelativeToRootFrame,
                                    false, ignoreRootScrollFrame);

  const EventRadiusPrefs* prefs = GetPrefsFor(aEvent->eventStructType);
  if (!prefs || !prefs->mEnabled || (target && IsElementClickable(target))) {
    return target;
  }

  
  
  if (aEvent->eventStructType == NS_MOUSE_EVENT &&
      prefs->mTouchOnly &&
      static_cast<const nsMouseEvent*>(aEvent)->inputSource !=
          nsIDOMMouseEvent::MOZ_SOURCE_TOUCH) {
      return target;
  }

  nsRect targetRect = GetTargetRect(aRootFrame, aPointRelativeToRootFrame, prefs);
  nsAutoTArray<nsIFrame*,8> candidates;
  nsresult rv = nsLayoutUtils::GetFramesForArea(aRootFrame, targetRect, candidates,
                                                false, ignoreRootScrollFrame);
  if (NS_FAILED(rv)) {
    return target;
  }

  
  
  
  
  
  nsIFrame* restrictToDescendants = target ?
    target->PresContext()->PresShell()->GetRootFrame() : aRootFrame;
  nsIFrame* closestClickable =
    GetClosest(aRootFrame, aPointRelativeToRootFrame, prefs,
               restrictToDescendants, candidates);
  return closestClickable ? closestClickable : target;
}

}
