





































#ifndef nsQueryContentEventHandler_h__
#define nsQueryContentEventHandler_h__

#include "nscore.h"
#include "nsCOMPtr.h"

#include "nsISelection.h"
#include "nsIRange.h"
#include "nsIContent.h"
#include "nsIDOMTreeWalker.h"

class nsPresContext;
class nsIPresShell;
class nsQueryContentEvent;
class nsCaret;
struct nsRect;









class NS_STACK_CLASS nsQueryContentEventHandler {
public:
  nsQueryContentEventHandler(nsPresContext *aPresContext);

  
  nsresult OnQuerySelectedText(nsQueryContentEvent* aEvent);
  
  nsresult OnQueryTextContent(nsQueryContentEvent* aEvent);
  
  nsresult OnQueryCharacterRect(nsQueryContentEvent* aEvent);
  
  nsresult OnQueryCaretRect(nsQueryContentEvent* aEvent);
protected:
  nsPresContext* mPresContext;
  nsIPresShell* mPresShell;
  nsCOMPtr<nsISelection> mSelection;
  nsCOMPtr<nsIRange> mFirstSelectedRange;
  nsCOMPtr<nsIContent> mRootContent;

  nsresult Init(nsQueryContentEvent* aEvent);

  
  

  
  nsresult GenerateFlatTextContent(nsIRange* aRange, nsAFlatString& aString);
  
  
  
  nsresult SetRangeFromFlatTextOffset(nsIRange* aRange,
                                      PRUint32 aNativeOffset,
                                      PRUint32 aNativeLength,
                                      PRBool aExpandToClusterBoundaries);
  
  nsresult GetFlatTextOffsetOfRange(nsIRange* aRange, PRUint32* aOffset);
  
  
  nsresult GetStartFrameAndOffset(nsIRange* aRange,
                                  nsIFrame** aFrame, PRInt32* aOffsetInFrame);
  
  nsresult ConvertToRootViewRelativeOffset(nsIFrame* aFrame, nsRect& aRect);
  
  
  nsresult QueryRectFor(nsQueryContentEvent* aEvent, nsIRange* aRange,
                        nsCaret* aCaret);
  
  
  nsresult ExpandToClusterBoundary(nsIContent* aContent, PRBool aForward,
                                   PRUint32* aXPOffset);
};

#endif 
