




#pragma once

#include "nsIWinMetroUtils.h"
#include "nsString.h"

namespace mozilla {
namespace widget {

class nsWinMetroUtils : public nsIWinMetroUtils
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWINMETROUTILS

  nsWinMetroUtils();
  virtual ~nsWinMetroUtils();
};

} 
} 
