




































 





 
#include "nsString.h"

#include "nsIFindService.h"



#define NS_FIND_SERVICE_CID \
 {0x5060b803, 0x340e, 0x11d5, {0xbe, 0x5b, 0xb3, 0xe0, 0x63, 0xec, 0x6a, 0x3c}}


#define NS_FIND_SERVICE_CONTRACTID \
 "@mozilla.org/find/find_service;1"


class nsFindService : public nsIFindService
{
public:

                      nsFindService();
  virtual             ~nsFindService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIFINDSERVICE
 
protected:

  nsString        mSearchString;
  nsString        mReplaceString;
  
  PRPackedBool    mFindBackwards;
  PRPackedBool    mWrapFind;
  PRPackedBool    mEntireWord;
  PRPackedBool    mMatchCase;
};
