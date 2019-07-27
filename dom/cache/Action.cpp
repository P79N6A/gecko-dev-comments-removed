





#include "mozilla/dom/cache/Action.h"

namespace mozilla {
namespace dom {
namespace cache {

void
Action::CancelOnInitiatingThread()
{
  NS_ASSERT_OWNINGTHREAD(Action);
  MOZ_ASSERT(!mCanceled);
  mCanceled = true;
}

Action::Action()
  : mCanceled(false)
{
}

Action::~Action()
{
}

bool
Action::IsCanceled() const
{
  return mCanceled;
}

} 
} 
} 
