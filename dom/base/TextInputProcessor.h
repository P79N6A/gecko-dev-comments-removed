




#ifndef mozilla_dom_textinputprocessor_h_
#define mozilla_dom_textinputprocessor_h_

#include "mozilla/TextEventDispatcherListener.h"
#include "nsITextInputProcessor.h"
#include "nsITextInputProcessorCallback.h"

namespace mozilla {

namespace widget{
class TextEventDispatcher;
} 

class TextInputProcessor MOZ_FINAL : public nsITextInputProcessor
                                   , public widget::TextEventDispatcherListener
{
  typedef mozilla::widget::IMENotification IMENotification;
  typedef mozilla::widget::TextEventDispatcher TextEventDispatcher;

public:
  TextInputProcessor();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITEXTINPUTPROCESSOR

  
  NS_IMETHOD NotifyIME(TextEventDispatcher* aTextEventDispatcher,
                       const IMENotification& aNotification) MOZ_OVERRIDE;
  NS_IMETHOD_(void)
    OnRemovedFrom(TextEventDispatcher* aTextEventDispatcher) MOZ_OVERRIDE;

protected:
  virtual ~TextInputProcessor();

private:
  nsresult BeginInputTransactionInternal(
             nsIDOMWindow* aWindow,
             nsITextInputProcessorCallback* aCallback,
             bool aForTests,
             bool& aSucceeded);
  nsresult CommitCompositionInternal(const nsAString* aCommitString = nullptr,
                                     bool* aSucceeded = nullptr);
  nsresult CancelCompositionInternal();
  nsresult IsValidStateForComposition();
  void UnlinkFromTextEventDispatcher();

  TextEventDispatcher* mDispatcher; 
  nsCOMPtr<nsITextInputProcessorCallback> mCallback;
  bool mForTests;
};

} 

#endif 
