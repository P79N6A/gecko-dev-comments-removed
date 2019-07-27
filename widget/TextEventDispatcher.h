




#ifndef mozilla_textcompositionsynthesizer_h_
#define mozilla_textcompositionsynthesizer_h_

#include "nsAutoPtr.h"
#include "nsString.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "mozilla/TextEventDispatcherListener.h"
#include "mozilla/TextRange.h"

class nsIWidget;

namespace mozilla {
namespace widget {

struct IMENotification;











class TextEventDispatcher MOZ_FINAL
{
  ~TextEventDispatcher()
  {
  }

  NS_INLINE_DECL_REFCOUNTING(TextEventDispatcher)

public:
  explicit TextEventDispatcher(nsIWidget* aWidget);

  













  nsresult BeginInputTransaction(TextEventDispatcherListener* aListener);
  nsresult BeginInputTransactionForTests(
             TextEventDispatcherListener* aListener);

  


  void OnDestroyWidget();

  









  nsresult GetState() const;

  



  bool IsComposing() const { return mIsComposing; }

  


  bool IsDispatchingEvent() const { return mDispatchingEvent > 0; }

  


  nsresult StartComposition(nsEventStatus& aStatus);

  






   nsresult CommitComposition(nsEventStatus& aStatus,
                              const nsAString* aCommitString = nullptr);

  





  nsresult SetPendingCompositionString(const nsAString& aString)
  {
    return mPendingComposition.SetString(aString);
  }

  









  nsresult AppendClauseToPendingComposition(uint32_t aLength,
                                            uint32_t aAttribute)
  {
    return mPendingComposition.AppendClause(aLength, aAttribute);
  }

  











  nsresult SetCaretInPendingComposition(uint32_t aOffset,
                                        uint32_t aLength)
  {
    return mPendingComposition.SetCaret(aOffset, aLength);
  }

  






  nsresult FlushPendingComposition(nsEventStatus& aStatus)
  {
    return mPendingComposition.Flush(this, aStatus);
  }

  


  void ClearPendingComposition()
  {
    mPendingComposition.Clear();
  }

  


  nsresult NotifyIME(const IMENotification& aIMENotification);

  













  bool DispatchKeyboardEvent(uint32_t aMessage,
                             const WidgetKeyboardEvent& aKeyboardEvent,
                             nsEventStatus& aStatus);

  














  bool MaybeDispatchKeypressEvents(const WidgetKeyboardEvent& aKeyboardEvent,
                                   nsEventStatus& aStatus);

private:
  
  
  
  
  nsIWidget* mWidget;
  
  
  
  
  
  nsWeakPtr mListener;

  
  
  
  
  class PendingComposition
  {
  public:
    PendingComposition();
    nsresult SetString(const nsAString& aString);
    nsresult AppendClause(uint32_t aLength, uint32_t aAttribute);
    nsresult SetCaret(uint32_t aOffset, uint32_t aLength);
    nsresult Flush(TextEventDispatcher* aDispatcher, nsEventStatus& aStatus);
    void Clear();

  private:
    nsAutoString mString;
    nsRefPtr<TextRangeArray> mClauses;
    TextRange mCaret;

    void EnsureClauseArray();
  };
  PendingComposition mPendingComposition;

  
  uint16_t mDispatchingEvent;

  bool mForTests;
  
  bool mIsComposing;

  
  
  static bool sDispatchKeyEventsDuringComposition;

  nsresult BeginInputTransactionInternal(
             TextEventDispatcherListener* aListener,
             bool aForTests);

  



  void InitEvent(WidgetGUIEvent& aEvent) const;

  


  nsresult DispatchEvent(nsIWidget* aWidget,
                         WidgetGUIEvent& aEvent,
                         nsEventStatus& aStatus);

  













  nsresult StartCompositionAutomaticallyIfNecessary(nsEventStatus& aStatus);

  




















  bool DispatchKeyboardEventInternal(uint32_t aMessage,
                                     const WidgetKeyboardEvent& aKeyboardEvent,
                                     nsEventStatus& aStatus,
                                     uint32_t aIndexOfKeypress = 0);
};

} 
} 

#endif 
