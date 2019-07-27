




#include "ActiveElementManager.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/Preferences.h"
#include "base/message_loop.h"
#include "base/task.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"
#include "nsStyleSet.h"

#define AEM_LOG(...)


namespace mozilla {
namespace layers {

static int32_t sActivationDelayMs = 100;
static bool sActivationDelayMsSet = false;

ActiveElementManager::ActiveElementManager()
  : mCanBePan(false),
    mCanBePanSet(false),
    mSetActiveTask(nullptr),
    mActiveElementUsesStyle(false)
{
  if (!sActivationDelayMsSet) {
    Preferences::AddIntVarCache(&sActivationDelayMs,
                                "ui.touch_activation.delay_ms",
                                sActivationDelayMs);
    sActivationDelayMsSet = true;
  }
}

ActiveElementManager::~ActiveElementManager() {}

void
ActiveElementManager::SetTargetElement(dom::EventTarget* aTarget)
{
  if (mTarget) {
    
    AEM_LOG("Multiple fingers on-screen, clearing target element\n");
    CancelTask();
    ResetActive();
    ResetTouchBlockState();
    return;
  }

  mTarget = do_QueryInterface(aTarget);
  AEM_LOG("Setting target element to %p\n", mTarget.get());
  TriggerElementActivation();
}

void
ActiveElementManager::HandleTouchStart(bool aCanBePan)
{
  AEM_LOG("Touch start, aCanBePan: %d\n", aCanBePan);
  if (mCanBePanSet) {
    
    AEM_LOG("Multiple fingers on-screen, clearing touch block state\n");
    CancelTask();
    ResetActive();
    ResetTouchBlockState();
    return;
  }

  mCanBePan = aCanBePan;
  mCanBePanSet = true;
  TriggerElementActivation();
}

void
ActiveElementManager::TriggerElementActivation()
{
  
  
  
  if (!(mTarget && mCanBePanSet)) {
    return;
  }

  
  
  if (!mCanBePan) {
    SetActive(mTarget);
  } else {
    MOZ_ASSERT(mSetActiveTask == nullptr);
    mSetActiveTask = NewRunnableMethod(
        this, &ActiveElementManager::SetActiveTask, mTarget);
    MessageLoop::current()->PostDelayedTask(
        FROM_HERE, mSetActiveTask, sActivationDelayMs);
    AEM_LOG("Scheduling mSetActiveTask %p\n", mSetActiveTask);
  }
}

void
ActiveElementManager::HandlePanStart()
{
  AEM_LOG("Handle pan start\n");

  
  
  CancelTask();
  ResetActive();
}

void
ActiveElementManager::HandleTouchEndEvent(bool aWasClick)
{
  AEM_LOG("Touch end event, aWasClick: %d\n", aWasClick);

  
  
  
  CancelTask();
  if (aWasClick) {
    SetActive(mTarget);
  } else {
    
    
    
    ResetActive();
  }

  ResetTouchBlockState();
}

void
ActiveElementManager::HandleTouchEnd()
{
  AEM_LOG("Touch end, clearing pan state\n");
  mCanBePanSet = false;
}

bool
ActiveElementManager::ActiveElementUsesStyle() const
{
  return mActiveElementUsesStyle;
}

static nsPresContext*
GetPresContextFor(nsIContent* aContent)
{
  if (!aContent) {
    return nullptr;
  }
  nsIPresShell* shell = aContent->OwnerDoc()->GetShell();
  if (!shell) {
    return nullptr;
  }
  return shell->GetPresContext();
}

static bool
ElementHasActiveStyle(dom::Element* aElement)
{
  nsPresContext* pc = GetPresContextFor(aElement);
  if (!pc) {
    return false;
  }
  nsStyleSet* styleSet = pc->StyleSet();
  for (dom::Element* e = aElement; e; e = e->GetParentElement()) {
    if (styleSet->HasStateDependentStyle(e, NS_EVENT_STATE_ACTIVE)) {
      AEM_LOG("Element %p's style is dependent on the active state\n", e);
      return true;
    }
  }
  AEM_LOG("Element %p doesn't use active styles\n", aElement);
  return false;
}

void
ActiveElementManager::SetActive(dom::Element* aTarget)
{
  AEM_LOG("Setting active %p\n", aTarget);

  if (nsPresContext* pc = GetPresContextFor(aTarget)) {
    pc->EventStateManager()->SetContentState(aTarget, NS_EVENT_STATE_ACTIVE);
    mActiveElementUsesStyle = ElementHasActiveStyle(aTarget);
  }
}

void
ActiveElementManager::ResetActive()
{
  AEM_LOG("Resetting active from %p\n", mTarget.get());

  
  if (mTarget) {
    dom::Element* root = mTarget->OwnerDoc()->GetDocumentElement();
    if (root) {
      AEM_LOG("Found root %p, making active\n", root);
      SetActive(root);
    }
  }
}

void
ActiveElementManager::ResetTouchBlockState()
{
  mTarget = nullptr;
  mCanBePanSet = false;
}

void
ActiveElementManager::SetActiveTask(dom::Element* aTarget)
{
  AEM_LOG("mSetActiveTask %p running\n", mSetActiveTask);

  
  
  
  mSetActiveTask = nullptr;
  SetActive(aTarget);
}

void
ActiveElementManager::CancelTask()
{
  AEM_LOG("Cancelling task %p\n", mSetActiveTask);

  if (mSetActiveTask) {
    mSetActiveTask->Cancel();
    mSetActiveTask = nullptr;
  }
}

}
}
