






#include "ImageContainerParent.h"

#include "nsThreadUtils.h"
#include "mozilla/layers/ImageHost.h"

namespace mozilla {
namespace layers {

ImageContainerParent::~ImageContainerParent()
{
  while (!mImageHosts.IsEmpty()) {
    mImageHosts[mImageHosts.Length() - 1]->SetImageContainer(nullptr);
  }
}

static void SendDeleteAndIgnoreResult(ImageContainerParent* self)
{
  unused << PImageContainerParent::Send__delete__(self);
}

bool ImageContainerParent::RecvAsyncDelete()
{
  MessageLoop::current()->PostTask(
    FROM_HERE, NewRunnableFunction(&SendDeleteAndIgnoreResult, this));

  return true;
}

}
}
