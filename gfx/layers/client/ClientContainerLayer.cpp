




#include "ClientContainerLayer.h"
#include "ClientLayerManager.h"         
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            

namespace mozilla {
namespace layers {

already_AddRefed<ContainerLayer>
ClientLayerManager::CreateContainerLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ClientContainerLayer> layer =
    new ClientContainerLayer(this);
  CREATE_SHADOW(Container);
  return layer.forget();
}

already_AddRefed<RefLayer>
ClientLayerManager::CreateRefLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ClientRefLayer> layer =
    new ClientRefLayer(this);
  CREATE_SHADOW(Ref);
  return layer.forget();
}

} 
} 
