




#ifndef mozilla_dom_compositionstringsynthesizer_h__
#define mozilla_dom_compositionstringsynthesizer_h__

#include "nsICompositionStringSynthesizer.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsWeakReference.h"
#include "mozilla/Attributes.h"
#include "mozilla/TextEvents.h"

class nsIWidget;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class CompositionStringSynthesizer MOZ_FINAL :
  public nsICompositionStringSynthesizer
{
public:
  CompositionStringSynthesizer(nsPIDOMWindow* aWindow);
  ~CompositionStringSynthesizer();

  NS_DECL_ISUPPORTS
  NS_DECL_NSICOMPOSITIONSTRINGSYNTHESIZER

private:
  nsWeakPtr mWindow; 
  nsString mString;
  nsAutoTArray<TextRange, 10> mClauses;
  TextRange mCaret;

  nsIWidget* GetWidget();
  void ClearInternal();
};

} 
} 

#endif 
