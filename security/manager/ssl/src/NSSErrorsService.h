









































#include "nsINSSErrorsService.h"

#include "nsIStringBundle.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace psm {

class NSSErrorsService : public nsINSSErrorsService
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSINSSERRORSSERVICE

public:
  nsresult Init();

private:
  nsCOMPtr<nsIStringBundle> mPIPNSSBundle;
  nsCOMPtr<nsIStringBundle> mNSSErrorsBundle;
};

} 
} 

#define NS_NSSERRORSSERVICE_CID \
  { 0x9ef18451, 0xa157, 0x4d17, { 0x81, 0x32, 0x47, 0xaf, 0xef, 0x21, 0x36, 0x89 } }
