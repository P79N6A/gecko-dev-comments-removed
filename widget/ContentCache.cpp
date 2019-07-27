






#include "mozilla/ContentCache.h"
#include "mozilla/TextEvents.h"
#include "nsIWidget.h"

namespace mozilla {

using namespace widget;

ContentCache::ContentCache()
  : mCompositionStart(UINT32_MAX)
  , mCompositionEventsDuringRequest(0)
  , mIsComposing(false)
  , mRequestedToCommitOrCancelComposition(false)
{
}

void
ContentCache::Clear()
{
  mText.Truncate();
}

void
ContentCache::SetText(const nsAString& aText)
{
  mText = aText;
}

void
ContentCache::SetSelection(uint32_t aAnchorOffset, uint32_t aFocusOffset)
{
  mSelection.mAnchor = aAnchorOffset;
  mSelection.mFocus = aFocusOffset;
}

bool
ContentCache::OnCompositionEvent(const WidgetCompositionEvent& aEvent)
{
  if (!aEvent.CausesDOMTextEvent()) {
    MOZ_ASSERT(aEvent.message == NS_COMPOSITION_START);
    mIsComposing = !aEvent.CausesDOMCompositionEndEvent();
    mCompositionStart = SelectionStart();
    
    if (mRequestedToCommitOrCancelComposition) {
      mCommitStringByRequest = aEvent.mData;
      mCompositionEventsDuringRequest++;
      return false;
    }
    return true;
  }

  
  

  
  
  
  
  
  
  if (mRequestedToCommitOrCancelComposition) {
    mCommitStringByRequest = aEvent.mData;
    mCompositionEventsDuringRequest++;
    return false;
  }

  
  
  if (!mIsComposing) {
    mCompositionStart = SelectionStart();
  }
  
  
  
  SetSelection(mCompositionStart + aEvent.mData.Length());
  mIsComposing = !aEvent.CausesDOMCompositionEndEvent();
  return true;
}

uint32_t
ContentCache::RequestToCommitComposition(nsIWidget* aWidget,
                                         bool aCancel,
                                         nsAString& aLastString)
{
  mRequestedToCommitOrCancelComposition = true;
  mCompositionEventsDuringRequest = 0;

  aWidget->NotifyIME(IMENotification(aCancel ? REQUEST_TO_CANCEL_COMPOSITION :
                                               REQUEST_TO_COMMIT_COMPOSITION));

  mRequestedToCommitOrCancelComposition = false;
  aLastString = mCommitStringByRequest;
  mCommitStringByRequest.Truncate(0);
  return mCompositionEventsDuringRequest;
}

} 
