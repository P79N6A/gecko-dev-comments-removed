



#ifndef mozilla_embedding_PrintSettingsDialogParent_h
#define mozilla_embedding_PrintSettingsDialogParent_h

#include "mozilla/embedding/PPrintSettingsDialogParent.h"


namespace mozilla {
namespace embedding {

class PrintSettingsDialogParent final : public PPrintSettingsDialogParent
{
public:
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  MOZ_IMPLICIT PrintSettingsDialogParent();

private:
  virtual ~PrintSettingsDialogParent();
};

} 
} 

#endif
