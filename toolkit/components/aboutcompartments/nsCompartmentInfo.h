




#ifndef nsCompartmentInfo_h
#define nsCompartmentInfo_h

#include "nsICompartmentInfo.h"

class nsCompartmentInfo : public nsICompartmentInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICOMPARTMENTINFO

  nsCompartmentInfo();

private:
  virtual ~nsCompartmentInfo();

protected:
};

#endif
