





#include "ProxyAccessible.h"
#include "DocAccessibleParent.h"
#include "mozilla/unused.h"
#include "mozilla/a11y/Platform.h"
#include "mozilla/a11y/Role.h"

namespace mozilla {
namespace a11y {

void
ProxyAccessible::Shutdown()
{
  NS_ASSERTION(!mOuterDoc, "Why do we still have a child doc?");

  
  
  if (!mOuterDoc) {
    uint32_t childCount = mChildren.Length();
    for (uint32_t idx = 0; idx < childCount; idx++)
      mChildren[idx]->Shutdown();
  } else {
    if (mChildren.Length() != 1)
      MOZ_CRASH("outer doc doesn't own adoc!");

    static_cast<DocAccessibleParent*>(mChildren[0])->Destroy();
  }

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

bool
ProxyAccessible::MustPruneChildren() const
{
  
  
  if (mRole != roles::MENUITEM && mRole != roles::COMBOBOX_OPTION
      && mRole != roles::OPTION && mRole != roles::ENTRY
      && mRole != roles::FLAT_EQUATION && mRole != roles::PASSWORD_TEXT
      && mRole != roles::PUSHBUTTON && mRole != roles::TOGGLE_BUTTON
      && mRole != roles::GRAPHIC && mRole != roles::SLIDER
      && mRole != roles::PROGRESSBAR && mRole != roles::SEPARATOR)
    return false;

  if (mChildren.Length() != 1)
    return false;

  return mChildren[0]->Role() == roles::TEXT_LEAF
    || mChildren[0]->Role() == roles::STATICTEXT;
}

uint64_t
ProxyAccessible::State() const
{
  uint64_t state = 0;
  unused << mDoc->SendState(mID, &state);
  return state;
}

void
ProxyAccessible::Name(nsString& aName) const
{
  unused << mDoc->SendName(mID, &aName);
}

void
ProxyAccessible::Value(nsString& aValue) const
{
  unused << mDoc->SendValue(mID, &aValue);
}

void
ProxyAccessible::Description(nsString& aDesc) const
{
  unused << mDoc->SendDescription(mID, &aDesc);
}

void
ProxyAccessible::Attributes(nsTArray<Attribute> *aAttrs) const
{
  unused << mDoc->SendAttributes(mID, aAttrs);
}

void
ProxyAccessible::TextSubstring(int32_t aStartOffset, int32_t aEndOfset,
                               nsString& aText) const
{
  unused << mDoc->SendTextSubstring(mID, aStartOffset, aEndOfset, &aText);
}

void
ProxyAccessible::GetTextAfterOffset(int32_t aOffset,
                                    AccessibleTextBoundary aBoundaryType,
                                    nsString& aText, int32_t* aStartOffset,
                                    int32_t* aEndOffset)
{
  unused << mDoc->SendGetTextAfterOffset(mID, aOffset, aBoundaryType,
                                         &aText, aStartOffset, aEndOffset);
}

void
ProxyAccessible::GetTextAtOffset(int32_t aOffset,
                                 AccessibleTextBoundary aBoundaryType,
                                 nsString& aText, int32_t* aStartOffset,
                                 int32_t* aEndOffset)
{
  unused << mDoc->SendGetTextAtOffset(mID, aOffset, aBoundaryType,
                                      &aText, aStartOffset, aEndOffset);
}

void
ProxyAccessible::GetTextBeforeOffset(int32_t aOffset,
                                     AccessibleTextBoundary aBoundaryType,
                                     nsString& aText, int32_t* aStartOffset,
                                     int32_t* aEndOffset)
{
  unused << mDoc->SendGetTextBeforeOffset(mID, aOffset, aBoundaryType,
                                          &aText, aStartOffset, aEndOffset);
}

}
}
