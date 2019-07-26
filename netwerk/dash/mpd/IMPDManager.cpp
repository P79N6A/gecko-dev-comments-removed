

















#include "nsIDOMElement.h"
#include "IMPDManager.h"
#include "nsDASHWebMODManager.h"
#include "nsDASHWebMODParser.h"


namespace mozilla {
namespace net {


IMPDManager*
IMPDManager::Create(DASHMPDProfile aProfile, nsIDOMElement* aRoot)
{
  switch(aProfile)
  {
    case WebMOnDemand:
      return CreateWebMOnDemandManager(aRoot);
    default:
      return nullptr;
  }
}


IMPDManager*
IMPDManager::CreateWebMOnDemandManager(nsIDOMElement* aRoot)
{
  
  nsDASHWebMODParser parser(aRoot);

  return new nsDASHWebMODManager(parser.Parse());
}



}
}
