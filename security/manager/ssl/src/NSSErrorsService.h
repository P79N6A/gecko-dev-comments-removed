



#ifndef NSSErrorsService_h
#define NSSErrorsService_h

#include "nsINSSErrorsService.h"
#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsIStringBundle.h"
#include "prerror.h"

namespace mozilla {
namespace psm {

class NSSErrorsService MOZ_FINAL : public nsINSSErrorsService
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSINSSERRORSSERVICE

public:
  nsresult Init();

private:
  
  
  
#ifdef _MSC_VER
  
  __pragma(warning(disable:4265))
#endif
  ~NSSErrorsService();

  nsCOMPtr<nsIStringBundle> mPIPNSSBundle;
  nsCOMPtr<nsIStringBundle> mNSSErrorsBundle;
};

bool IsNSSErrorCode(PRErrorCode code);
nsresult GetXPCOMFromNSSError(PRErrorCode code);

} 
} 

#define NS_NSSERRORSSERVICE_CID \
  { 0x9ef18451, 0xa157, 0x4d17, { 0x81, 0x32, 0x47, 0xaf, 0xef, 0x21, 0x36, 0x89 } }

#endif 
