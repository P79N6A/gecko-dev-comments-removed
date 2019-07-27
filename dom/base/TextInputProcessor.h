




#ifndef mozilla_dom_textinputprocessor_h_
#define mozilla_dom_textinputprocessor_h_

#include "nsITextInputProcessor.h"

namespace mozilla {

namespace widget{
class TextEventDispatcher;
} 

class TextInputProcessor MOZ_FINAL : public nsITextInputProcessor
{
  typedef mozilla::widget::TextEventDispatcher TextEventDispatcher;

public:
  TextInputProcessor();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITEXTINPUTPROCESSOR

private:
  ~TextInputProcessor();

  nsresult InitInternal(nsIDOMWindow* aWindow,
                        bool aForTests,
                        bool& aSucceeded);
  nsresult IsValidStateForComposition() const;

  TextEventDispatcher* mDispatcher; 
  bool mIsInitialized;
  bool mForTests;
};

} 

#endif 
