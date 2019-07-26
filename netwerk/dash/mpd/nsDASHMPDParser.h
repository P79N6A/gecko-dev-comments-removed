

























#ifndef __DASHMPDPARSER_H__
#define __DASHMPDPARSER_H__

#include "nsAutoPtr.h"
#include "nsNetUtil.h"
#include "nsIPrincipal.h"
#include "nsIDOMElement.h"
#include "IMPDManager.h"

namespace mozilla {
namespace net {

class nsDASHMPDParser
{
public:
  
  
  
  nsDASHMPDParser(char*         aMPDData,
                  uint32_t      aDataLength,
                  nsIPrincipal* aPrincipal,
                  nsIURI*       aURI);
 
  ~nsDASHMPDParser();

  
  
  nsresult  Parse(IMPDManager**   aMPDManager,
                  DASHMPDProfile* aProfile);
private:
  
  nsresult  GetProfile(nsIDOMElement* aRoot,
                       DASHMPDProfile &profile);
  
  void      PrintDOMElements(nsIDOMElement* aRoot);
  
  void      PrintDOMElement(nsIDOMElement* aElem, int32_t aOffset);

  
  nsAutoPtr<char const>   mData;
  
  uint32_t                mDataLength;
  
  nsCOMPtr<nsIPrincipal>  mPrincipal;
  
  nsCOMPtr<nsIURI>        mURI;
};

}
}

#endif 
