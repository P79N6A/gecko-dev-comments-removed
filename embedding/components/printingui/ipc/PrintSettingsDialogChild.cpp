



#include "PrintSettingsDialogChild.h"

using mozilla::unused;

namespace mozilla {
namespace embedding {

PrintSettingsDialogChild::PrintSettingsDialogChild()
: mReturned(false)
{
  MOZ_COUNT_CTOR(PrintSettingsDialogChild);
}

PrintSettingsDialogChild::~PrintSettingsDialogChild()
{
  MOZ_COUNT_DTOR(PrintSettingsDialogChild);
}

bool
PrintSettingsDialogChild::Recv__delete__(const nsresult& aResult,
                                         const PrintData& aData)
{
  mResult = aResult;
  mData = aData;
  mReturned = true;
  return true;
}

} 
} 
