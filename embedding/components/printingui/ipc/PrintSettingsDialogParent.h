



#ifndef mozilla_embedding_PrintSettingsDialogParent_h
#define mozilla_embedding_PrintSettingsDialogParent_h

#include "mozilla/embedding/PPrintSettingsDialogParent.h"


namespace mozilla {
namespace embedding {

class PrintSettingsDialogParent MOZ_FINAL : public PPrintSettingsDialogParent
{
public:
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  MOZ_IMPLICIT PrintSettingsDialogParent();

private:
  virtual ~PrintSettingsDialogParent();
};

} 
} 

#endif
