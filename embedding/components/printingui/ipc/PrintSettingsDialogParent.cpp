



#include "PrintSettingsDialogParent.h"


namespace mozilla {
namespace embedding {

MOZ_IMPLICIT PrintSettingsDialogParent::PrintSettingsDialogParent()
{
  MOZ_COUNT_CTOR(PrintSettingsDialogParent);
}

MOZ_IMPLICIT PrintSettingsDialogParent::~PrintSettingsDialogParent()
{
  MOZ_COUNT_DTOR(PrintSettingsDialogParent);
}

void
PrintSettingsDialogParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

} 
} 

