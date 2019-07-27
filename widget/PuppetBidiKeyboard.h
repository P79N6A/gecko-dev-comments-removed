






#ifndef mozilla_widget_PuppetBidiKeyboard_h_
#define mozilla_widget_PuppetBidiKeyboard_h_

#include "nsIBidiKeyboard.h"

namespace mozilla {
namespace widget {

class PuppetBidiKeyboard final : public nsIBidiKeyboard
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIBIDIKEYBOARD

  PuppetBidiKeyboard();

  void SetIsLangRTL(bool aIsLangRTL);

private:
  ~PuppetBidiKeyboard();

  bool mIsLangRTL;
};

} 
} 

#endif 
