





#include "RootAccessibleWrap.h"

#include "nsMai.h"

using namespace mozilla::a11y;

GtkWindowAccessible::GtkWindowAccessible(AtkObject* aAccessible) :
  DummyAccessible()
{
  g_object_ref(aAccessible);
  mAtkObject = aAccessible;
}

GtkWindowAccessible::~GtkWindowAccessible()
{
  g_object_unref(mAtkObject);
  mAtkObject = nullptr;
}
