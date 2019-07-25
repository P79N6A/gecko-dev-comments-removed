






































#ifndef nsContentEventHandler_h__
#define nsContentEventHandler_h__

#include "nscore.h"
#include "nsCOMPtr.h"

#include "nsISelection.h"
#include "nsIRange.h"
#include "nsIContent.h"
#include "nsIDOMTreeWalker.h"

class nsPresContext;
class nsIPresShell;
class nsQueryContentEvent;
class nsSelectionEvent;
class nsCaret;
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
  nsCOMPtr<nsIRange> mFirstSelectedRange;
  nsCOMPtr<nsIContent> mRootContent;

  nsresult Init(nsQueryContentEvent* aEvent);
  nsresult Init(nsSelectionEvent* aEvent);

  
  nsresult InitCommon();

public:
  
  

  
  static nsresult GetFlatTextOffsetOfRange(nsIContent* aRootContent,
                                           nsINode* aNode,
                                           PRInt32 aNodeOffset,
                                           PRUint32* aOffset);
  static nsresult GetFlatTextOffsetOfRange(nsIContent* aRootContent,
                                           nsIRange* aRange,
                                           PRUint32* aOffset);
protected:
  
  
  
  nsresult SetRangeFromFlatTextOffset(nsIRange* aRange,
                                      PRUint32 aNativeOffset,
                                      PRUint32 aNativeLength,
                                      bool aExpandToClusterBoundaries);
  
  
  nsresult GetStartFrameAndOffset(nsIRange* aRange,
                                  nsIFrame** aFrame,
                                  PRInt32* aOffsetInFrame);
  
  nsresult ConvertToRootViewRelativeOffset(nsIFrame* aFrame,
                                           nsRect& aRect);
  
  
  nsresult ExpandToClusterBoundary(nsIContent* aContent, bool aForward,
                                   PRUint32* aXPOffset);
};

#endif 
