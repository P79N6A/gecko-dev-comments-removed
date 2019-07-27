



#include "PrintSettingsDialogParent.h"


namespace mozilla {
namespace embedding {

PrintSettingsDialogParent::PrintSettingsDialogParent()
{
  MOZ_COUNT_CTOR(PrintSettingsDialogParent);
}

PrintSettingsDialogParent::~PrintSettingsDialogParent()
{
  MOZ_COUNT_DTOR(PrintSettingsDialogParent);
}

void
PrintSettingsDialogParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

} 
} 

