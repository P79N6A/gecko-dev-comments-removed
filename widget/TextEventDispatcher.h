




#ifndef mozilla_textcompositionsynthesizer_h_
#define mozilla_textcompositionsynthesizer_h_

#include "nsAutoPtr.h"
#include "nsString.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "mozilla/TextRange.h"

class nsIWidget;

namespace mozilla {
namespace widget {











class TextEventDispatcher MOZ_FINAL
{
  ~TextEventDispatcher()
  {
  }

  NS_INLINE_DECL_REFCOUNTING(TextEventDispatcher)

public:
  explicit TextEventDispatcher(nsIWidget* aWidget);

  





  nsresult Init();
  nsresult InitForTests();

  


  void OnDestroyWidget();

  





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

private:
  
  
  
  
  nsIWidget* mWidget;

  
  
  
  
  class PendingComposition
  {
  public:
    PendingComposition();
    nsresult SetString(const nsAString& aString);
    nsresult AppendClause(uint32_t aLength, uint32_t aAttribute);
    nsresult SetCaret(uint32_t aOffset, uint32_t aLength);
    nsresult Flush(const TextEventDispatcher* aDispatcher,
                   nsEventStatus& aStatus);
    void Clear();

  private:
    nsAutoString mString;
    nsRefPtr<TextRangeArray> mClauses;
    TextRange mCaret;

    void EnsureClauseArray();
  };
  PendingComposition mPendingComposition;

  bool mInitialized;
  bool mForTests;
};

} 
} 

#endif 
