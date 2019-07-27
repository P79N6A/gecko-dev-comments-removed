




#ifndef mozilla_dom_compositionstringsynthesizer_h__
#define mozilla_dom_compositionstringsynthesizer_h__

#include "nsICompositionStringSynthesizer.h"
#include "nsString.h"
#include "nsWeakReference.h"
#include "mozilla/Attributes.h"
#include "mozilla/TextRange.h"

class nsIWidget;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class CompositionStringSynthesizer MOZ_FINAL :
  public nsICompositionStringSynthesizer
{
public:
  CompositionStringSynthesizer(nsPIDOMWindow* aWindow);

  NS_DECL_ISUPPORTS
  NS_DECL_NSICOMPOSITIONSTRINGSYNTHESIZER

private:
  ~CompositionStringSynthesizer();

  nsWeakPtr mWindow; 
  nsString mString;
  nsRefPtr<TextRangeArray> mClauses;
  TextRange mCaret;

  nsIWidget* GetWidget();
  void ClearInternal();
};

} 
} 

#endif 
