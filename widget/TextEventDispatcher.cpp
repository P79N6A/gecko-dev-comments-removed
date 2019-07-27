




#include "mozilla/Preferences.h"
#include "mozilla/TextEvents.h"
#include "mozilla/TextEventDispatcher.h"
#include "nsIDocShell.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsIWidget.h"
#include "nsPIDOMWindow.h"
#include "nsView.h"

namespace mozilla {
namespace widget {





bool TextEventDispatcher::sDispatchKeyEventsDuringComposition = false;

TextEventDispatcher::TextEventDispatcher(nsIWidget* aWidget)
  : mWidget(aWidget)
  , mForTests(false)
  , mIsComposing(false)
{
  MOZ_RELEASE_ASSERT(mWidget, "aWidget must not be nullptr");

  static bool sInitialized = false;
  if (!sInitialized) {
    Preferences::AddBoolVarCache(
      &sDispatchKeyEventsDuringComposition,
      "dom.keyboardevent.dispatch_during_composition",
      false);
    sInitialized = true;
  }
}

nsresult
TextEventDispatcher::BeginInputTransaction(
                       TextEventDispatcherListener* aListener)
{
  return BeginInputTransactionInternal(aListener, false);
}

nsresult
TextEventDispatcher::BeginInputTransactionForTests(
                       TextEventDispatcherListener* aListener)
{
  return BeginInputTransactionInternal(aListener, true);
}

nsresult
TextEventDispatcher::BeginInputTransactionInternal(
                       TextEventDispatcherListener* aListener,
                       bool aForTests)
{
  if (NS_WARN_IF(!aListener)) {
    return NS_ERROR_INVALID_ARG;
  }
  nsCOMPtr<TextEventDispatcherListener> listener = do_QueryReferent(mListener);
  if (listener) {
    if (listener == aListener && mForTests == aForTests) {
      return NS_OK;
    }
    
    if (IsComposing()) {
      return NS_ERROR_ALREADY_INITIALIZED;
    }
  }
  mListener = do_GetWeakReference(aListener);
  mForTests = aForTests;
  if (listener && listener != aListener) {
    listener->OnRemovedFrom(this);
  }
  return NS_OK;
}

void
TextEventDispatcher::OnDestroyWidget()
{
  mWidget = nullptr;
  mPendingComposition.Clear();
  nsCOMPtr<TextEventDispatcherListener> listener = do_QueryReferent(mListener);
  mListener = nullptr;
  if (listener) {
    listener->OnRemovedFrom(this);
  }
}

nsresult
TextEventDispatcher::GetState() const
{
  nsCOMPtr<TextEventDispatcherListener> listener = do_QueryReferent(mListener);
  if (!listener) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  if (!mWidget || mWidget->Destroyed()) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  return NS_OK;
}

void
TextEventDispatcher::InitEvent(WidgetGUIEvent& aEvent) const
{
  aEvent.time = PR_IntervalNow();
  aEvent.refPoint = LayoutDeviceIntPoint(0, 0);
  aEvent.mFlags.mIsSynthesizedForTests = mForTests;
}

nsresult
TextEventDispatcher::StartComposition(nsEventStatus& aStatus)
{
  aStatus = nsEventStatus_eIgnore;

  nsresult rv = GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (NS_WARN_IF(mIsComposing)) {
    return NS_ERROR_FAILURE;
  }

  mIsComposing = true;
  nsCOMPtr<nsIWidget> widget(mWidget);
  WidgetCompositionEvent compositionStartEvent(true, NS_COMPOSITION_START,
                                               widget);
  InitEvent(compositionStartEvent);
  rv = widget->DispatchEvent(&compositionStartEvent, aStatus);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
TextEventDispatcher::StartCompositionAutomaticallyIfNecessary(
                       nsEventStatus& aStatus)
{
  if (IsComposing()) {
    return NS_OK;
  }

  nsresult rv = StartComposition(aStatus);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  if (!IsComposing()) {
    aStatus = nsEventStatus_eConsumeNoDefault;
    return NS_OK;
  }

  
  
  
  rv = GetState();
  if (NS_FAILED(rv)) {
    MOZ_ASSERT(rv != NS_ERROR_NOT_INITIALIZED,
               "aDispatcher must still be initialized in this case");
    aStatus = nsEventStatus_eConsumeNoDefault;
    return NS_OK; 
  }

  aStatus = nsEventStatus_eIgnore;
  return NS_OK;
}

nsresult
TextEventDispatcher::CommitComposition(nsEventStatus& aStatus,
                                       const nsAString* aCommitString)
{
  aStatus = nsEventStatus_eIgnore;

  nsresult rv = GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  
  if (NS_WARN_IF(!IsComposing() &&
                 (!aCommitString || aCommitString->IsEmpty()))) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIWidget> widget(mWidget);
  rv = StartCompositionAutomaticallyIfNecessary(aStatus);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  if (aStatus == nsEventStatus_eConsumeNoDefault) {
    return NS_OK;
  }

  
  mIsComposing = false;

  uint32_t message = aCommitString ? NS_COMPOSITION_COMMIT :
                                     NS_COMPOSITION_COMMIT_AS_IS;
  WidgetCompositionEvent compositionCommitEvent(true, message, widget);
  InitEvent(compositionCommitEvent);
  if (message == NS_COMPOSITION_COMMIT) {
    compositionCommitEvent.mData = *aCommitString;
  }
  rv = widget->DispatchEvent(&compositionCommitEvent, aStatus);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
TextEventDispatcher::NotifyIME(const IMENotification& aIMENotification)
{
  nsCOMPtr<TextEventDispatcherListener> listener = do_QueryReferent(mListener);
  if (!listener) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  nsresult rv = listener->NotifyIME(this, aIMENotification);
  
  
  
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  return rv;
}

bool
TextEventDispatcher::DispatchKeyboardEvent(
                       uint32_t aMessage,
                       const WidgetKeyboardEvent& aKeyboardEvent,
                       nsEventStatus& aStatus)
{
  return DispatchKeyboardEventInternal(aMessage, aKeyboardEvent, aStatus);
}

bool
TextEventDispatcher::DispatchKeyboardEventInternal(
                       uint32_t aMessage,
                       const WidgetKeyboardEvent& aKeyboardEvent,
                       nsEventStatus& aStatus,
                       uint32_t aIndexOfKeypress)
{
  MOZ_ASSERT(aMessage == NS_KEY_DOWN || aMessage == NS_KEY_UP ||
             aMessage == NS_KEY_PRESS, "Invalid aMessage value");
  nsresult rv = GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }

  
  if (aMessage == NS_KEY_PRESS && !aKeyboardEvent.ShouldCauseKeypressEvents()) {
    return false;
  }

  
  if (IsComposing()) {
    
    
    
    if (!sDispatchKeyEventsDuringComposition || aMessage == NS_KEY_PRESS) {
      return false;
    }
    
    
    
  }

  nsCOMPtr<nsIWidget> widget(mWidget);

  WidgetKeyboardEvent keyEvent(true, aMessage, widget);
  InitEvent(keyEvent);
  keyEvent.AssignKeyEventData(aKeyboardEvent, false);

  if (aStatus == nsEventStatus_eConsumeNoDefault) {
    
    
    
    keyEvent.mFlags.mDefaultPrevented = true;
  }

  
  if (aMessage == NS_KEY_DOWN || aMessage == NS_KEY_UP) {
    MOZ_ASSERT(!aIndexOfKeypress,
      "aIndexOfKeypress must be 0 for either NS_KEY_DOWN or NS_KEY_UP");
    
    keyEvent.charCode = 0;
  } else if (keyEvent.mKeyNameIndex != KEY_NAME_INDEX_USE_STRING) {
    MOZ_ASSERT(!aIndexOfKeypress,
      "aIndexOfKeypress must be 0 for NS_KEY_PRESS of non-printable key");
    
    
    keyEvent.charCode = 0;
  } else {
    MOZ_RELEASE_ASSERT(
      !aIndexOfKeypress || aIndexOfKeypress < keyEvent.mKeyValue.Length(),
      "aIndexOfKeypress must be 0 - mKeyValue.Length() - 1");
    keyEvent.keyCode = 0;
    wchar_t ch =
      keyEvent.mKeyValue.IsEmpty() ? 0 : keyEvent.mKeyValue[aIndexOfKeypress];
    keyEvent.charCode = static_cast<uint32_t>(ch);
    if (ch) {
      keyEvent.mKeyValue.Assign(ch);
    } else {
      keyEvent.mKeyValue.Truncate();
    }
  }
  if (aMessage == NS_KEY_UP) {
    
    keyEvent.mIsRepeat = false;
  }
  
  keyEvent.mIsComposing = false;
  
  
  keyEvent.mNativeKeyEvent = nullptr;
  
  
  keyEvent.mPluginEvent.Clear();
  

  widget->DispatchEvent(&keyEvent, aStatus);
  return true;
}

bool
TextEventDispatcher::MaybeDispatchKeypressEvents(
                       const WidgetKeyboardEvent& aKeyboardEvent,
                       nsEventStatus& aStatus)
{
  
  if (aStatus == nsEventStatus_eConsumeNoDefault) {
    return false;
  }

  
  
  
  
  size_t keypressCount =
    aKeyboardEvent.mKeyNameIndex != KEY_NAME_INDEX_USE_STRING ?
      1 : std::max(static_cast<nsAString::size_type>(1),
                   aKeyboardEvent.mKeyValue.Length());
  bool isDispatched = false;
  bool consumed = false;
  for (size_t i = 0; i < keypressCount; i++) {
    aStatus = nsEventStatus_eIgnore;
    if (!DispatchKeyboardEventInternal(NS_KEY_PRESS, aKeyboardEvent,
                                       aStatus, i)) {
      
      break;
    }
    isDispatched = true;
    if (!consumed) {
      consumed = (aStatus == nsEventStatus_eConsumeNoDefault);
    }
  }

  
  if (consumed) {
    aStatus = nsEventStatus_eConsumeNoDefault;
  }

  return isDispatched;
}





TextEventDispatcher::PendingComposition::PendingComposition()
{
  Clear();
}

void
TextEventDispatcher::PendingComposition::Clear()
{
  mString.Truncate();
  mClauses = nullptr;
  mCaret.mRangeType = 0;
}

void
TextEventDispatcher::PendingComposition::EnsureClauseArray()
{
  if (mClauses) {
    return;
  }
  mClauses = new TextRangeArray();
}

nsresult
TextEventDispatcher::PendingComposition::SetString(const nsAString& aString)
{
  mString = aString;
  return NS_OK;
}

nsresult
TextEventDispatcher::PendingComposition::AppendClause(uint32_t aLength,
                                                      uint32_t aAttribute)
{
  if (NS_WARN_IF(!aLength)) {
    return NS_ERROR_INVALID_ARG;
  }

  switch (aAttribute) {
    case NS_TEXTRANGE_RAWINPUT:
    case NS_TEXTRANGE_SELECTEDRAWTEXT:
    case NS_TEXTRANGE_CONVERTEDTEXT:
    case NS_TEXTRANGE_SELECTEDCONVERTEDTEXT: {
      EnsureClauseArray();
      TextRange textRange;
      textRange.mStartOffset =
        mClauses->IsEmpty() ? 0 : mClauses->LastElement().mEndOffset;
      textRange.mEndOffset = textRange.mStartOffset + aLength;
      textRange.mRangeType = aAttribute;
      mClauses->AppendElement(textRange);
      return NS_OK;
    }
    default:
      return NS_ERROR_INVALID_ARG;
  }
}

nsresult
TextEventDispatcher::PendingComposition::SetCaret(uint32_t aOffset,
                                                  uint32_t aLength)
{
  mCaret.mStartOffset = aOffset;
  mCaret.mEndOffset = mCaret.mStartOffset + aLength;
  mCaret.mRangeType = NS_TEXTRANGE_CARETPOSITION;
  return NS_OK;
}

nsresult
TextEventDispatcher::PendingComposition::Flush(TextEventDispatcher* aDispatcher,
                                               nsEventStatus& aStatus)
{
  aStatus = nsEventStatus_eIgnore;

  nsresult rv = aDispatcher->GetState();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (mClauses && !mClauses->IsEmpty() &&
      mClauses->LastElement().mEndOffset != mString.Length()) {
    NS_WARNING("Sum of length of the all clauses must be same as the string "
               "length");
    Clear();
    return NS_ERROR_ILLEGAL_VALUE;
  }
  if (mCaret.mRangeType == NS_TEXTRANGE_CARETPOSITION) {
    if (mCaret.mEndOffset > mString.Length()) {
      NS_WARNING("Caret position is out of the composition string");
      Clear();
      return NS_ERROR_ILLEGAL_VALUE;
    }
    EnsureClauseArray();
    mClauses->AppendElement(mCaret);
  }

  nsCOMPtr<nsIWidget> widget(aDispatcher->mWidget);
  WidgetCompositionEvent compChangeEvent(true, NS_COMPOSITION_CHANGE, widget);
  aDispatcher->InitEvent(compChangeEvent);
  compChangeEvent.mData = mString;
  if (mClauses) {
    MOZ_ASSERT(!mClauses->IsEmpty(),
               "mClauses must be non-empty array when it's not nullptr");
    compChangeEvent.mRanges = mClauses;
  }

  
  
  
  Clear();

  rv = aDispatcher->StartCompositionAutomaticallyIfNecessary(aStatus);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  if (aStatus == nsEventStatus_eConsumeNoDefault) {
    return NS_OK;
  }
  rv = widget->DispatchEvent(&compChangeEvent, aStatus);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

} 
} 
