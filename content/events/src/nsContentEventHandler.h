




#ifndef nsContentEventHandler_h__
#define nsContentEventHandler_h__

#include "nsCOMPtr.h"

#include "nsISelection.h"
#include "nsRange.h"
#include "mozilla/EventForwards.h"

class nsCaret;
class nsPresContext;

struct nsRect;









class MOZ_STACK_CLASS nsContentEventHandler {
public:
  nsContentEventHandler(nsPresContext *aPresContext);

  
  nsresult OnQuerySelectedText(mozilla::WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryTextContent(mozilla::WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryCaretRect(mozilla::WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryTextRect(mozilla::WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryEditorRect(mozilla::WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryContentState(mozilla::WidgetQueryContentEvent* aEvent);
  
  nsresult OnQuerySelectionAsTransferable(
             mozilla::WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryCharacterAtPoint(mozilla::WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryDOMWidgetHittest(mozilla::WidgetQueryContentEvent* aEvent);

  
  nsresult OnSelectionEvent(mozilla::WidgetSelectionEvent* aEvent);

protected:
  nsPresContext* mPresContext;
  nsCOMPtr<nsIPresShell> mPresShell;
  nsCOMPtr<nsISelection> mSelection;
  nsRefPtr<nsRange> mFirstSelectedRange;
  nsCOMPtr<nsIContent> mRootContent;

  nsresult Init(mozilla::WidgetQueryContentEvent* aEvent);
  nsresult Init(mozilla::WidgetSelectionEvent* aEvent);

  
  nsresult InitCommon();

public:
  
  

  
  static nsresult GetFlatTextOffsetOfRange(nsIContent* aRootContent,
                                           nsINode* aNode,
                                           int32_t aNodeOffset,
                                           uint32_t* aOffset);
  static nsresult GetFlatTextOffsetOfRange(nsIContent* aRootContent,
                                           nsRange* aRange,
                                           uint32_t* aOffset);
  
  static uint32_t GetNativeTextLength(nsIContent* aContent,
                                      uint32_t aMaxLength = UINT32_MAX);
protected:
  
  
  
  nsresult SetRangeFromFlatTextOffset(nsRange* aRange,
                                      uint32_t aNativeOffset,
                                      uint32_t aNativeLength,
                                      bool aExpandToClusterBoundaries,
                                      uint32_t* aNewNativeOffset = nullptr);
  
  
  nsresult GetStartFrameAndOffset(nsRange* aRange,
                                  nsIFrame** aFrame,
                                  int32_t* aOffsetInFrame);
  
  nsresult ConvertToRootViewRelativeOffset(nsIFrame* aFrame,
                                           nsRect& aRect);
  
  
  nsresult ExpandToClusterBoundary(nsIContent* aContent, bool aForward,
                                   uint32_t* aXPOffset);
};

#endif 
