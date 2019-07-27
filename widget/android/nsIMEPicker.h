



#ifndef __nsIMEPicker
#define __nsIMEPicker

#include "nsIIMEPicker.h"

class nsIMEPicker final : public nsIIMEPicker
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIMEPICKER

  nsIMEPicker();

private:
  ~nsIMEPicker();
};

#endif
