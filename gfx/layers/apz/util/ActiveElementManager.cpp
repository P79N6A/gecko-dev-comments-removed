




#include "ActiveElementManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "inIDOMUtils.h"
#include "base/message_loop.h"
#include "base/task.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"

#define AEM_LOG(...)


namespace mozilla {
namespace layers {

static int32_t sActivationDelayMs = 100;
static bool sActivationDelayMsSet = false;

ActiveElementManager::ActiveElementManager()
  : mDomUtils(services::GetInDOMUtils()),
    mCanBePan(false),
    mCanBePanSet(false),
    mSetActiveTask(nullptr)
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
ActiveElementManager::HandleTouchEnd(bool aWasClick)
{
  AEM_LOG("Touch end, aWasClick: %d\n", aWasClick);

  
  
  
  CancelTask();
  if (aWasClick) {
    SetActive(mTarget);
  } else {
    
    
    
    ResetActive();
  }

  ResetTouchBlockState();
}

void
ActiveElementManager::SetActive(dom::Element* aTarget)
{
  AEM_LOG("Setting active %p\n", aTarget);
  if (mDomUtils) {
    nsCOMPtr<nsIDOMElement> target = do_QueryInterface(aTarget);
    mDomUtils->SetContentState(target, NS_EVENT_STATE_ACTIVE.GetInternalValue());
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
