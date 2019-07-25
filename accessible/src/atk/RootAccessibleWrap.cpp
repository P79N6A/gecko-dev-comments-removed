





#include "RootAccessibleWrap.h"

#include "nsMai.h"

using namespace mozilla::a11y;

NativeRootAccessibleWrap::NativeRootAccessibleWrap(AtkObject* aAccessible):
  RootAccessible(nullptr, nullptr, nullptr)
{
  
  
  mFlags |= eIsDefunct;

  g_object_ref(aAccessible);
  mAtkObject = aAccessible;
}

NativeRootAccessibleWrap::~NativeRootAccessibleWrap()
{
  g_object_unref(mAtkObject);
  mAtkObject = nullptr;
}
