










































#include "mozilla/Util.h"

#include "nsGkAtoms.h"
#include "nsStaticAtom.h"

using namespace mozilla;


#define GK_ATOM(name_, value_) nsIAtom* nsGkAtoms::name_;
#include "nsGkAtomList.h"
#undef GK_ATOM

#define GK_ATOM(name_, value_) NS_STATIC_ATOM_BUFFER(name_##_buffer, value_)
#include "nsGkAtomList.h"
#undef GK_ATOM

static const nsStaticAtom GkAtoms_info[] = {
#define GK_ATOM(name_, value_) NS_STATIC_ATOM(name_##_buffer, &nsGkAtoms::name_),
#include "nsGkAtomList.h"
#undef GK_ATOM
};

void nsGkAtoms::AddRefAtoms()
{
  NS_RegisterStaticAtoms(GkAtoms_info, ArrayLength(GkAtoms_info));
}

