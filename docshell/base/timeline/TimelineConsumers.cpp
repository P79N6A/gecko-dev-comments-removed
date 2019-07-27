





#include "mozilla/TimelineConsumers.h"

namespace mozilla {

unsigned long TimelineConsumers::sActiveConsumers = 0;

void
TimelineConsumers::AddConsumer()
{
  sActiveConsumers++;
}

void
TimelineConsumers::RemoveConsumer()
{
  sActiveConsumers--;
}

bool
TimelineConsumers::IsEmpty()
{
  return sActiveConsumers == 0;
}

} 
