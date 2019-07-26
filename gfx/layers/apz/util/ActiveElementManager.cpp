




#include "ActiveElementManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "inIDOMUtils.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventTarget.h"
#include "base/message_loop.h"
#include "base/task.h"

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
ActiveElementManager::SetTargetElement(nsIDOMEventTarget* aTarget)
{
  if (mTarget) {
    
    CancelTask();
    ResetActive();
    ResetTouchBlockState();
    return;
  }

  mTarget = do_QueryInterface(aTarget);
  TriggerElementActivation();
}

void
ActiveElementManager::HandleTouchStart(bool aCanBePan)
{
  if (mCanBePanSet) {
    
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
  }
}

void
ActiveElementManager::HandlePanStart()
{
  
  
  CancelTask();
  ResetActive();
}

void
ActiveElementManager::HandleTouchEnd(bool aWasClick)
{
  
  
  
  CancelTask();
  if (aWasClick) {
    SetActive(mTarget);
  }

  ResetTouchBlockState();
}

void
ActiveElementManager::SetActive(nsIDOMElement* aTarget)
{
  if (mDomUtils) {
    mDomUtils->SetContentState(aTarget, NS_EVENT_STATE_ACTIVE.GetInternalValue());;
  }
}

void
ActiveElementManager::ResetActive()
{
  
  if (mTarget) {
    nsCOMPtr<nsIDOMDocument> doc;
    mTarget->GetOwnerDocument(getter_AddRefs(doc));
    if (doc) {
      nsCOMPtr<nsIDOMElement> root;
      doc->GetDocumentElement(getter_AddRefs(root));
      if (root) {
        SetActive(root);
      }
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
ActiveElementManager::SetActiveTask(nsIDOMElement* aTarget)
{
  
  
  
  mSetActiveTask = nullptr;
  SetActive(aTarget);
}

void
ActiveElementManager::CancelTask()
{
  if (mSetActiveTask) {
    mSetActiveTask->Cancel();
    mSetActiveTask = nullptr;
  }
}

}
}
