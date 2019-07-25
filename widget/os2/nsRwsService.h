






#ifndef nsRwsService_h__
#define nsRwsService_h__

#include "nsIRwsService.h"
#include "rws.h"


#define NS_RWSSERVICE_CID \
{ 0xe314efd1, 0xf4ef,0x49e0, { 0xbd, 0x98, 0x12, 0xd4, 0xe8, 0x7a, 0x63, 0xa7 } }

#define NS_RWSSERVICE_CONTRACTID "@mozilla.org/rwsos2;1"



NS_IMETHODIMP nsRwsServiceConstructor(nsISupports *aOuter, REFNSIID aIID,
                                      void **aResult);



class ExtCache;

class nsRwsService : public nsIRwsService, public nsIObserver
{
public:
  NS_DECL_NSIRWSSERVICE
  NS_DECL_NSIOBSERVER
  NS_DECL_ISUPPORTS

  nsRwsService();

private:
  ~nsRwsService();

protected:
  static nsresult RwsConvert(uint32_t type, uint32_t value, uint32_t *result);
  static nsresult RwsConvert(uint32_t type, uint32_t value, nsAString& result);

  ExtCache *mExtCache;
};



#endif 
