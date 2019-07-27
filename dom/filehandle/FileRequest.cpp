





#include "FileRequest.h"

#include "MainThreadUtils.h"
#include "mozilla/Assertions.h"

namespace mozilla {
namespace dom {

FileRequestBase::FileRequestBase()
{
  MOZ_ASSERT(NS_IsMainThread());
}

FileRequestBase::~FileRequestBase()
{
  MOZ_ASSERT(NS_IsMainThread());
}

} 
} 
