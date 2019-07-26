






#ifndef mozilla_a11y_HyperTextAccessibleWrap_h__
#define mozilla_a11y_HyperTextAccessibleWrap_h__

#include "HyperTextAccessible.h"
#include "ia2AccessibleEditableText.h"
#include "ia2AccessibleHypertext.h"

namespace mozilla {
template<class T> class StaticAutoPtr;
template<class T> class StaticRefPtr;

namespace a11y {

class HyperTextAccessibleWrap : public HyperTextAccessible,
                                public ia2AccessibleHypertext,
                                public ia2AccessibleEditableText
{
public:
  HyperTextAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    HyperTextAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult HandleAccEvent(AccEvent* aEvent);

protected:
  virtual nsresult GetModifiedText(bool aGetInsertedText, nsAString& aText,
                                   uint32_t *aStartOffset,
                                   uint32_t *aEndOffset);

  static StaticRefPtr<Accessible> sLastTextChangeAcc;
  static StaticAutoPtr<nsString> sLastTextChangeString;
  static bool sLastTextChangeWasInsert;
  static uint32_t sLastTextChangeStart;
  static uint32_t sLastTextChangeEnd;

  friend void PlatformInit();
};

} 
} 

#endif
