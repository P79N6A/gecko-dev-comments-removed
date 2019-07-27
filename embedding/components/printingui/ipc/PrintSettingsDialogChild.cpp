



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
PrintSettingsDialogChild::Recv__delete__(const PrintDataOrNSResult& aData)
{
  if (aData.type() == PrintDataOrNSResult::Tnsresult) {
    mResult = aData.get_nsresult();
    MOZ_ASSERT(NS_FAILED(mResult), "expected a failure result");
  } else {
    mResult = NS_OK;
    mData = aData.get_PrintData();
  }
  mReturned = true;
  return true;
}

} 
} 
