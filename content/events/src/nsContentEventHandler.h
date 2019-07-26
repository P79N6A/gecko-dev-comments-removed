




#ifndef nsContentEventHandler_h__
#define nsContentEventHandler_h__

#include "nscore.h"
#include "nsCOMPtr.h"

#include "nsISelection.h"
#include "nsRange.h"
#include "nsIDOMTreeWalker.h"

class nsCaret;
class nsIContent;
class nsIPresShell;
class nsPresContext;
class nsQueryContentEvent;
class nsSelectionEvent;

struct nsRect;









class NS_STACK_CLASS nsContentEventHandler {
public:
  nsContentEventHandler(nsPresContext *aPresContext);

  
  nsresult OnQuerySelectedText(nsQueryContentEvent* aEvent);
  
  nsresult OnQueryTextContent(nsQueryContentEvent* aEvent);
  
  nsresult OnQueryCaretRect(nsQueryContentEvent* aEvent);
  
  nsresult OnQueryTextRect(nsQueryContentEvent* aEvent);
  
  nsresult OnQueryEditorRect(nsQueryContentEvent* aEvent);
  
  nsresult OnQueryContentState(nsQueryContentEvent* aEvent);
  
  nsresult OnQuerySelectionAsTransferable(nsQueryContentEvent* aEvent);
  
  nsresult OnQueryCharacterAtPoint(nsQueryContentEvent* aEvent);
  
  nsresult OnQueryDOMWidgetHittest(nsQueryContentEvent* aEvent);

  
  nsresult OnSelectionEvent(nsSelectionEvent* aEvent);

protected:
  nsPresContext* mPresContext;
  nsCOMPtr<nsIPresShell> mPresShell;
  nsCOMPtr<nsISelection> mSelection;
  nsRefPtr<nsRange> mFirstSelectedRange;
  nsCOMPtr<nsIContent> mRootContent;

  nsresult Init(nsQueryContentEvent* aEvent);
  nsresult Init(nsSelectionEvent* aEvent);

  
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
                                      bool aExpandToClusterBoundaries);
  
  
  nsresult GetStartFrameAndOffset(nsRange* aRange,
                                  nsIFrame** aFrame,
                                  int32_t* aOffsetInFrame);
  
  nsresult ConvertToRootViewRelativeOffset(nsIFrame* aFrame,
                                           nsRect& aRect);
  
  
  nsresult ExpandToClusterBoundary(nsIContent* aContent, bool aForward,
                                   uint32_t* aXPOffset);
};

#endif 
