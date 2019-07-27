




#ifndef mozilla_dom_compositionstringsynthesizer_h__
#define mozilla_dom_compositionstringsynthesizer_h__

#include "nsICompositionStringSynthesizer.h"

#include "mozilla/TextEventDispatcher.h"

namespace mozilla {
namespace dom {

class CompositionStringSynthesizer MOZ_FINAL :
  public nsICompositionStringSynthesizer
{
  typedef mozilla::widget::TextEventDispatcher TextEventDispatcher;

public:
  explicit CompositionStringSynthesizer(TextEventDispatcher* aDispatcher);

  NS_DECL_ISUPPORTS
  NS_DECL_NSICOMPOSITIONSTRINGSYNTHESIZER

private:
  ~CompositionStringSynthesizer();

  nsRefPtr<TextEventDispatcher> mDispatcher;
};

} 
} 

#endif 
