





#include "ProxyAccessible.h"
#include "DocAccessibleParent.h"
#include "mozilla/a11y/Platform.h"

namespace mozilla {
namespace a11y {

void
ProxyAccessible::Shutdown()
{
  MOZ_ASSERT(!mOuterDoc);

  uint32_t childCount = mChildren.Length();
  for (uint32_t idx = 0; idx < childCount; idx++)
    mChildren[idx]->Shutdown();

  mChildren.Clear();
  ProxyDestroyed(this);
  mDoc->RemoveAccessible(this);
}

void
ProxyAccessible::SetChildDoc(DocAccessibleParent* aParent)
{
  if (aParent) {
    MOZ_ASSERT(mChildren.IsEmpty());
    mChildren.AppendElement(aParent);
    mOuterDoc = true;
  } else {
    MOZ_ASSERT(mChildren.Length() == 1);
    mChildren.Clear();
    mOuterDoc = false;
  }
}
}
}
