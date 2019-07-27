






#ifndef mozilla_ContentCache_h
#define mozilla_ContentCache_h

#include <stdint.h>

#include "nsString.h"

namespace mozilla {







class ContentCache final
{
public:
  void Clear();

  void SetText(const nsAString& aText);
  const nsString& Text() const { return mText; }
  uint32_t TextLength() const { return mText.Length(); }

private:
  
  nsString mText;
};

} 

#endif 
