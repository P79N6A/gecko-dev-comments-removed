




#include "mozilla/EventForwards.h"
#include "mozilla/TextEventDispatcher.h"
#include "mozilla/TextEvents.h"
#include "mozilla/TextInputProcessor.h"
#include "nsIDocShell.h"
#include "nsIWidget.h"
#include "nsPIDOMWindow.h"
#include "nsPresContext.h"

using namespace mozilla::widget;

namespace mozilla {





class TextInputProcessorNotification MOZ_FINAL :
        public nsITextInputProcessorNotification
{
public:
  explicit TextInputProcessorNotification(const char* aType)
    : mType(aType)
  {
  }

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetType(nsACString& aType) MOZ_OVERRIDE MOZ_FINAL
  {
    aType = mType;
    return NS_OK;
  }

protected:
  ~TextInputProcessorNotification() { }

private:
  nsAutoCString mType;

  TextInputProcessorNotification() { }
};

NS_IMPL_ISUPPORTS(TextInputProcessorNotification,
                  nsITextInputProcessorNotification)





NS_IMPL_ISUPPORTS(TextInputProcessor,
                  nsITextInputProcessor,
                  TextEventDispatcherListener,
                  nsISupportsWeakReference)

TextInputProcessor::TextInputProcessor()
  : mDispatcher(nullptr)
  , mForTests(false)
{
}

TextInputProcessor::~TextInputProcessor()
{
  if (mDispatcher && mDispatcher->IsComposing()) {
    
    
    
    if (NS_SUCCEEDED(IsValidStateForComposition())) {
      nsRefPtr<TextEventDispatcher> kungFuDeathGrip(mDispatcher);
      nsEventStatus status = nsEventStatus_eIgnore;
      mDispatcher->CommitComposition(status, &EmptyString());
    }
  }
}

NS_IMETHODIMP
TextInputProcessor::BeginInputTransaction(
                      nsIDOMWindow* aWindow,
                      nsITextInputProcessorCallback* aCallback,
                      bool* aSucceeded)
{
  MOZ_RELEASE_ASSERT(aSucceeded, "aSucceeded must not be nullptr");
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  if (NS_WARN_IF(!aCallback)) {
    *aSucceeded = false;
    return NS_ERROR_INVALID_ARG;
  }
  return BeginInputTransactionInternal(aWindow, aCallback, false, *aSucceeded);
}

NS_IMETHODIMP
TextInputProcessor::BeginInputTransactionForTests(
                      nsIDOMWindow* aWindow,
                      nsITextInputProcessorCallback* aCallback,
                      uint8_t aOptionalArgc,
                      bool* aSucceeded)
{
  MOZ_RELEASE_ASSERT(aSucceeded, "aSucceeded must not be nullptr");
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  nsITextInputProcessorCallback* callback =
    aOptionalArgc >= 1 ? aCallback : nullptr;
  return BeginInputTransactionInternal(aWindow, callback, true, *aSucceeded);
}

nsresult
TextInputProcessor::BeginInputTransactionInternal(
                      nsIDOMWindow* aWindow,
                      nsITextInputProcessorCallback* aCallback,
                      bool aForTests,
                      bool& aSucceeded)
{
  aSucceeded = false;
  if (NS_WARN_IF(!aWindow)) {
    return NS_ERROR_INVALID_ARG;
  }
  nsCOMPtr<nsPIDOMWindow> pWindow(do_QueryInterface(aWindow));
  if (NS_WARN_IF(!pWindow)) {
    return NS_ERROR_INVALID_ARG;
  }
  nsCOMPtr<nsIDocShell> docShell(pWindow->GetDocShell());
  if (NS_WARN_IF(!docShell)) {
    return NS_ERROR_FAILURE;
  }
  nsRefPtr<nsPresContext> presContext;
  nsresult rv = docShell->GetPresContext(getter_AddRefs(presContext));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  if (NS_WARN_IF(!presContext)) {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIWidget> widget = presContext->GetRootWidget();
  if (NS_WARN_IF(!widget)) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<TextEventDispatcher> dispatcher = widget->GetTextEventDispatcher();
  MOZ_RELEASE_ASSERT(dispatcher, "TextEventDispatcher must not be null");

  
  
  
  if (mDispatcher && dispatcher == mDispatcher && aCallback == mCallback &&
      aForTests == mForTests) {
    aSucceeded = true;
    return NS_OK;
  }

  
  if (mDispatcher && mDispatcher->IsComposing()) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  
  
  
  if (dispatcher->IsComposing()) {
    return NS_OK;
  }

  
  
  UnlinkFromTextEventDispatcher();

  if (aForTests) {
    rv = dispatcher->BeginInputTransactionForTests(this);
  } else {
    rv = dispatcher->BeginInputTransaction(this);
  }

  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mDispatcher = dispatcher;
  mCallback = aCallback;
  mForTests = aForTests;
  aSucceeded = true;
  return NS_OK;
}

void
TextInputProcessor::UnlinkFromTextEventDispatcher()
{
  mDispatcher = nullptr;
  mForTests = false;
  if (mCallback) {
    nsCOMPtr<nsITextInputProcessorCallback> callback(mCallback);
    mCallback = nullptr;

    nsRefPtr<TextInputProcessorNotification> notification =
      new TextInputProcessorNotification("notify-end-input-transaction");
    bool result = false;
    callback->OnNotify(this, notification, &result);
  }
}

nsresult
TextInputProcessor::IsValidStateForComposition()
{
  if (NS_WARN_IF(!mDispatcher)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsresult rv = mDispatcher->GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
TextInputProcessor::StartComposition(bool* aSucceeded)
{
  MOZ_RELEASE_ASSERT(aSucceeded, "aSucceeded must not be nullptr");
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  *aSucceeded = false;
  nsRefPtr<TextEventDispatcher> kungfuDeathGrip(mDispatcher);
  nsresult rv = IsValidStateForComposition();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  nsEventStatus status = nsEventStatus_eIgnore;
  rv = mDispatcher->StartComposition(status);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  *aSucceeded = status != nsEventStatus_eConsumeNoDefault &&
                  mDispatcher && mDispatcher->IsComposing();
  return NS_OK;
}

NS_IMETHODIMP
TextInputProcessor::SetPendingCompositionString(const nsAString& aString)
{
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  nsRefPtr<TextEventDispatcher> kungfuDeathGrip(mDispatcher);
  nsresult rv = IsValidStateForComposition();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  return mDispatcher->SetPendingCompositionString(aString);
}

NS_IMETHODIMP
TextInputProcessor::AppendClauseToPendingComposition(uint32_t aLength,
                                                     uint32_t aAttribute)
{
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  nsRefPtr<TextEventDispatcher> kungfuDeathGrip(mDispatcher);
  switch (aAttribute) {
    case ATTR_RAW_CLAUSE:
    case ATTR_SELECTED_RAW_CLAUSE:
    case ATTR_CONVERTED_CLAUSE:
    case ATTR_SELECTED_CLAUSE:
      break;
    default:
      return NS_ERROR_INVALID_ARG;
  }
  nsresult rv = IsValidStateForComposition();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  return mDispatcher->AppendClauseToPendingComposition(aLength, aAttribute);
}

NS_IMETHODIMP
TextInputProcessor::SetCaretInPendingComposition(uint32_t aOffset)
{
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  nsRefPtr<TextEventDispatcher> kungfuDeathGrip(mDispatcher);
  nsresult rv = IsValidStateForComposition();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  return mDispatcher->SetCaretInPendingComposition(aOffset, 0);
}

NS_IMETHODIMP
TextInputProcessor::FlushPendingComposition(bool* aSucceeded)
{
  MOZ_RELEASE_ASSERT(aSucceeded, "aSucceeded must not be nullptr");
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  *aSucceeded = false;
  nsRefPtr<TextEventDispatcher> kungfuDeathGrip(mDispatcher);
  nsresult rv = IsValidStateForComposition();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  nsEventStatus status = nsEventStatus_eIgnore;
  rv = mDispatcher->FlushPendingComposition(status);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  *aSucceeded = status != nsEventStatus_eConsumeNoDefault;
  return rv;
}

NS_IMETHODIMP
TextInputProcessor::CommitComposition(const nsAString& aCommitString,
                                      uint8_t aOptionalArgc,
                                      bool* aSucceeded)
{
  MOZ_RELEASE_ASSERT(aSucceeded, "aSucceeded must not be nullptr");
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  const nsAString* commitString =
    aOptionalArgc >= 1 ? &aCommitString : nullptr;
  return CommitCompositionInternal(commitString, aSucceeded);
}

nsresult
TextInputProcessor::CommitCompositionInternal(const nsAString* aCommitString,
                                              bool* aSucceeded)
{
  if (aSucceeded) {
    *aSucceeded = false;
  }
  nsRefPtr<TextEventDispatcher> kungfuDeathGrip(mDispatcher);
  nsresult rv = IsValidStateForComposition();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  nsEventStatus status = nsEventStatus_eIgnore;
  rv = mDispatcher->CommitComposition(status, aCommitString);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  if (aSucceeded) {
    *aSucceeded = status != nsEventStatus_eConsumeNoDefault;
  }
  return rv;
}

NS_IMETHODIMP
TextInputProcessor::CancelComposition()
{
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  return CancelCompositionInternal();
}

nsresult
TextInputProcessor::CancelCompositionInternal()
{
  nsRefPtr<TextEventDispatcher> kungfuDeathGrip(mDispatcher);
  nsresult rv = IsValidStateForComposition();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  nsEventStatus status = nsEventStatus_eIgnore;
  return mDispatcher->CommitComposition(status, &EmptyString());
}

NS_IMETHODIMP
TextInputProcessor::NotifyIME(TextEventDispatcher* aTextEventDispatcher,
                              const IMENotification& aNotification)
{
  
  if (!mDispatcher) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  MOZ_ASSERT(aTextEventDispatcher == mDispatcher,
             "Wrong TextEventDispatcher notifies this");
  NS_ASSERTION(mForTests || mCallback,
               "mCallback can be null only when IME is initialized for tests");
  if (mCallback) {
    nsRefPtr<TextInputProcessorNotification> notification;
    switch (aNotification.mMessage) {
      case REQUEST_TO_COMMIT_COMPOSITION: {
        NS_ASSERTION(aTextEventDispatcher->IsComposing(),
                     "Why is this requested without composition?");
        notification = new TextInputProcessorNotification("request-to-commit");
        break;
      }
      case REQUEST_TO_CANCEL_COMPOSITION: {
        NS_ASSERTION(aTextEventDispatcher->IsComposing(),
                     "Why is this requested without composition?");
        notification = new TextInputProcessorNotification("request-to-cancel");
        break;
      }
      case NOTIFY_IME_OF_FOCUS:
        notification = new TextInputProcessorNotification("notify-focus");
        break;
      case NOTIFY_IME_OF_BLUR:
        notification = new TextInputProcessorNotification("notify-blur");
        break;
      default:
        return NS_ERROR_NOT_IMPLEMENTED;
    }
    MOZ_RELEASE_ASSERT(notification);
    bool result = false;
    nsresult rv = mCallback->OnNotify(this, notification, &result);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
    return result ? NS_OK : NS_ERROR_FAILURE;
  }

  switch (aNotification.mMessage) {
    case REQUEST_TO_COMMIT_COMPOSITION: {
      NS_ASSERTION(aTextEventDispatcher->IsComposing(),
                   "Why is this requested without composition?");
      CommitCompositionInternal();
      return NS_OK;
    }
    case REQUEST_TO_CANCEL_COMPOSITION: {
      NS_ASSERTION(aTextEventDispatcher->IsComposing(),
                   "Why is this requested without composition?");
      CancelCompositionInternal();
      return NS_OK;
    }
    default:
      return NS_ERROR_NOT_IMPLEMENTED;
  }
}

NS_IMETHODIMP_(void)
TextInputProcessor::OnRemovedFrom(TextEventDispatcher* aTextEventDispatcher)
{
  
  if (!mDispatcher) {
    return;
  }
  MOZ_ASSERT(aTextEventDispatcher == mDispatcher,
             "Wrong TextEventDispatcher notifies this");
  UnlinkFromTextEventDispatcher();
}

nsresult
TextInputProcessor::PrepareKeyboardEventToDispatch(
                      WidgetKeyboardEvent& aKeyboardEvent,
                      uint32_t aKeyFlags)
{
  if (NS_WARN_IF(aKeyboardEvent.mCodeNameIndex == CODE_NAME_INDEX_USE_STRING)) {
    return NS_ERROR_INVALID_ARG;
  }
  if ((aKeyFlags & KEY_NON_PRINTABLE_KEY) &&
      NS_WARN_IF(aKeyboardEvent.mKeyNameIndex == KEY_NAME_INDEX_USE_STRING)) {
    return NS_ERROR_INVALID_ARG;
  }
  if ((aKeyFlags & KEY_FORCE_PRINTABLE_KEY) &&
      aKeyboardEvent.mKeyNameIndex != KEY_NAME_INDEX_USE_STRING) {
    aKeyboardEvent.GetDOMKeyName(aKeyboardEvent.mKeyValue);
    aKeyboardEvent.mKeyNameIndex = KEY_NAME_INDEX_USE_STRING;
  }
  if (aKeyFlags & KEY_KEEP_KEY_LOCATION_STANDARD) {
    
    
    
    if (NS_WARN_IF(aKeyboardEvent.location)) {
      return NS_ERROR_INVALID_ARG;
    }
  } else if (!aKeyboardEvent.location) {
    
    
    aKeyboardEvent.location =
      WidgetKeyboardEvent::ComputeLocationFromCodeValue(
        aKeyboardEvent.mCodeNameIndex);
  }

  if (aKeyFlags & KEY_KEEP_KEYCODE_ZERO) {
    
    
    
    if (NS_WARN_IF(aKeyboardEvent.keyCode)) {
      return NS_ERROR_INVALID_ARG;
    }
  } else if (!aKeyboardEvent.keyCode &&
             aKeyboardEvent.mKeyNameIndex > KEY_NAME_INDEX_Unidentified &&
             aKeyboardEvent.mKeyNameIndex < KEY_NAME_INDEX_USE_STRING) {
    
    
    
    aKeyboardEvent.keyCode =
      WidgetKeyboardEvent::ComputeKeyCodeFromKeyNameIndex(
        aKeyboardEvent.mKeyNameIndex);
  }
  return NS_OK;
}

NS_IMETHODIMP
TextInputProcessor::Keydown(nsIDOMKeyEvent* aDOMKeyEvent,
                            uint32_t aKeyFlags,
                            uint8_t aOptionalArgc,
                            bool* aDoDefault)
{
  MOZ_RELEASE_ASSERT(aDoDefault, "aDoDefault must not be nullptr");
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  if (!aOptionalArgc) {
    aKeyFlags = 0;
  }
  *aDoDefault = false;
  if (NS_WARN_IF(!aDOMKeyEvent)) {
    return NS_ERROR_INVALID_ARG;
  }

  WidgetKeyboardEvent* originalKeyEvent =
    aDOMKeyEvent->GetInternalNSEvent()->AsKeyboardEvent();
  if (NS_WARN_IF(!originalKeyEvent)) {
    return NS_ERROR_INVALID_ARG;
  }
  
  WidgetKeyboardEvent keyEvent(*originalKeyEvent);
  nsresult rv = PrepareKeyboardEventToDispatch(keyEvent, aKeyFlags);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  *aDoDefault = !(aKeyFlags & KEY_DEFAULT_PREVENTED);

  if (WidgetKeyboardEvent::GetModifierForKeyName(keyEvent.mKeyNameIndex)) {
    ModifierKeyData modifierKeyData(keyEvent);
    if (WidgetKeyboardEvent::IsLockableModifier(keyEvent.mKeyNameIndex)) {
      
      
      ToggleModifierKey(modifierKeyData);
    } else {
      
      
      ActivateModifierKey(modifierKeyData);
    }
  }
  keyEvent.modifiers = GetActiveModifiers();

  nsRefPtr<TextEventDispatcher> kungfuDeathGrip(mDispatcher);
  rv = IsValidStateForComposition();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsEventStatus status = *aDoDefault ? nsEventStatus_eIgnore :
                                       nsEventStatus_eConsumeNoDefault;
  if (!mDispatcher->DispatchKeyboardEvent(NS_KEY_DOWN, keyEvent, status)) {
    
    
    return NS_OK;
  }

  mDispatcher->MaybeDispatchKeypressEvents(keyEvent, status);

  *aDoDefault = (status != nsEventStatus_eConsumeNoDefault);
  return NS_OK;
}

NS_IMETHODIMP
TextInputProcessor::Keyup(nsIDOMKeyEvent* aDOMKeyEvent,
                          uint32_t aKeyFlags,
                          uint8_t aOptionalArgc,
                          bool* aDoDefault)
{
  MOZ_RELEASE_ASSERT(aDoDefault, "aDoDefault must not be nullptr");
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  if (!aOptionalArgc) {
    aKeyFlags = 0;
  }
  *aDoDefault = false;
  if (NS_WARN_IF(!aDOMKeyEvent)) {
    return NS_ERROR_INVALID_ARG;
  }

  WidgetKeyboardEvent* originalKeyEvent =
    aDOMKeyEvent->GetInternalNSEvent()->AsKeyboardEvent();
  if (NS_WARN_IF(!originalKeyEvent)) {
    return NS_ERROR_INVALID_ARG;
  }
  
  WidgetKeyboardEvent keyEvent(*originalKeyEvent);
  nsresult rv = PrepareKeyboardEventToDispatch(keyEvent, aKeyFlags);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  *aDoDefault = !(aKeyFlags & KEY_DEFAULT_PREVENTED);

  if (WidgetKeyboardEvent::GetModifierForKeyName(keyEvent.mKeyNameIndex) &&
      !WidgetKeyboardEvent::IsLockableModifier(keyEvent.mKeyNameIndex)) {
    
    
    InactivateModifierKey(ModifierKeyData(keyEvent));
  }
  keyEvent.modifiers = GetActiveModifiers();

  nsRefPtr<TextEventDispatcher> kungfuDeathGrip(mDispatcher);
  rv = IsValidStateForComposition();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsEventStatus status = *aDoDefault ? nsEventStatus_eIgnore :
                                       nsEventStatus_eConsumeNoDefault;
  mDispatcher->DispatchKeyboardEvent(NS_KEY_UP, keyEvent, status);
  *aDoDefault = (status != nsEventStatus_eConsumeNoDefault);
  return NS_OK;
}

NS_IMETHODIMP
TextInputProcessor::GetModifierState(const nsAString& aModifierKeyName,
                                     bool* aActive)
{
  MOZ_RELEASE_ASSERT(aActive, "aActive must not be null");
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  if (!mModifierKeyDataArray) {
    *aActive = false;
    return NS_OK;
  }
  Modifiers activeModifiers = mModifierKeyDataArray->GetActiveModifiers();
  Modifiers modifier = WidgetInputEvent::GetModifier(aModifierKeyName);
  *aActive = ((activeModifiers & modifier) != 0);
  return NS_OK;
}

NS_IMETHODIMP
TextInputProcessor::ShareModifierStateOf(nsITextInputProcessor* aOther)
{
  MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome());
  if (!aOther) {
    mModifierKeyDataArray = nullptr;
    return NS_OK;
  }
  TextInputProcessor* other = static_cast<TextInputProcessor*>(aOther);
  if (!other->mModifierKeyDataArray) {
    other->mModifierKeyDataArray = new ModifierKeyDataArray();
  }
  mModifierKeyDataArray = other->mModifierKeyDataArray;
  return NS_OK;
}




TextInputProcessor::ModifierKeyData::ModifierKeyData(
  const WidgetKeyboardEvent& aKeyboardEvent)
  : mKeyNameIndex(aKeyboardEvent.mKeyNameIndex)
  , mCodeNameIndex(aKeyboardEvent.mCodeNameIndex)
{
  mModifier = WidgetKeyboardEvent::GetModifierForKeyName(mKeyNameIndex);
  MOZ_ASSERT(mModifier, "mKeyNameIndex must be a modifier key name");
}




Modifiers
TextInputProcessor::ModifierKeyDataArray::GetActiveModifiers() const
{
  Modifiers result = MODIFIER_NONE;
  for (uint32_t i = 0; i < Length(); i++) {
    result |= ElementAt(i).mModifier;
  }
  return result;
}

void
TextInputProcessor::ModifierKeyDataArray::ActivateModifierKey(
  const TextInputProcessor::ModifierKeyData& aModifierKeyData)
{
  if (Contains(aModifierKeyData)) {
    return;
  }
  AppendElement(aModifierKeyData);
}

void
TextInputProcessor::ModifierKeyDataArray::InactivateModifierKey(
  const TextInputProcessor::ModifierKeyData& aModifierKeyData)
{
  RemoveElement(aModifierKeyData);
}

void
TextInputProcessor::ModifierKeyDataArray::ToggleModifierKey(
  const TextInputProcessor::ModifierKeyData& aModifierKeyData)
{
  auto index = IndexOf(aModifierKeyData);
  if (index == NoIndex) {
    AppendElement(aModifierKeyData);
    return;
  }
  RemoveElementAt(index);
}

} 
