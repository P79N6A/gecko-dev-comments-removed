




#ifndef mozilla_ContentEventHandler_h_
#define mozilla_ContentEventHandler_h_

#include "mozilla/EventForwards.h"
#include "nsCOMPtr.h"
#include "nsISelection.h"
#include "nsRange.h"

class nsPresContext;

struct nsRect;

namespace mozilla {

enum LineBreakType
{
  LINE_BREAK_TYPE_NATIVE,
  LINE_BREAK_TYPE_XP
};









class MOZ_STACK_CLASS ContentEventHandler
{
public:
  explicit ContentEventHandler(nsPresContext* aPresContext);

  
  nsresult OnQuerySelectedText(WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryTextContent(WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryCaretRect(WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryTextRect(WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryEditorRect(WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryContentState(WidgetQueryContentEvent* aEvent);
  
  nsresult OnQuerySelectionAsTransferable(WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryCharacterAtPoint(WidgetQueryContentEvent* aEvent);
  
  nsresult OnQueryDOMWidgetHittest(WidgetQueryContentEvent* aEvent);

  
  nsresult OnSelectionEvent(WidgetSelectionEvent* aEvent);

protected:
  nsPresContext* mPresContext;
  nsCOMPtr<nsIPresShell> mPresShell;
  nsCOMPtr<nsISelection> mSelection;
  nsRefPtr<nsRange> mFirstSelectedRange;
  nsCOMPtr<nsIContent> mRootContent;

  nsresult Init(WidgetQueryContentEvent* aEvent);
  nsresult Init(WidgetSelectionEvent* aEvent);

  nsresult InitBasic();
  nsresult InitCommon();

public:
  
  

  
  static nsresult GetFlatTextOffsetOfRange(nsIContent* aRootContent,
                                           nsINode* aNode,
                                           int32_t aNodeOffset,
                                           uint32_t* aOffset,
                                           LineBreakType aLineBreakType);
  static nsresult GetFlatTextOffsetOfRange(nsIContent* aRootContent,
                                           nsRange* aRange,
                                           uint32_t* aOffset,
                                           LineBreakType aLineBreakType);
  
  
  
  static uint32_t GetNativeTextLength(nsIContent* aContent,
                                      uint32_t aStartOffset,
                                      uint32_t aEndOffset);
  
  static uint32_t GetNativeTextLength(nsIContent* aContent,
                                      uint32_t aMaxLength = UINT32_MAX);
protected:
  static uint32_t GetTextLength(nsIContent* aContent,
                                LineBreakType aLineBreakType,
                                uint32_t aMaxLength = UINT32_MAX);
  static LineBreakType GetLineBreakType(WidgetQueryContentEvent* aEvent);
  static LineBreakType GetLineBreakType(WidgetSelectionEvent* aEvent);
  static LineBreakType GetLineBreakType(bool aUseNativeLineBreak);
  
  nsIContent* GetFocusedContent();
  
  bool IsPlugin(nsIContent* aContent);
  
  nsresult QueryContentRect(nsIContent* aContent,
                            WidgetQueryContentEvent* aEvent);
  
  
  
  nsresult SetRangeFromFlatTextOffset(nsRange* aRange,
                                      uint32_t aOffset,
                                      uint32_t aLength,
                                      LineBreakType aLineBreakType,
                                      bool aExpandToClusterBoundaries,
                                      uint32_t* aNewOffset = nullptr);
  
  
  nsresult GetStartFrameAndOffset(nsRange* aRange,
                                  nsIFrame** aFrame,
                                  int32_t* aOffsetInFrame);
  
  nsresult ConvertToRootViewRelativeOffset(nsIFrame* aFrame,
                                           nsRect& aRect);
  
  
  nsresult ExpandToClusterBoundary(nsIContent* aContent, bool aForward,
                                   uint32_t* aXPOffset);
};

} 

#endif 
