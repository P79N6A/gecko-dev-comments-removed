



#ifndef __nsIMEPicker
#define __nsIMEPicker

#include "nsIIMEPicker.h"

class nsIMEPicker MOZ_FINAL : public nsIIMEPicker
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIMEPICKER

  nsIMEPicker();

private:
  ~nsIMEPicker();
};

#endif
