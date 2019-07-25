



































#include "mozilla/Util.h"

#include "gfxAtoms.h"
#include "nsStaticAtom.h"
#include "nsMemory.h"

using namespace mozilla;

#define GFX_ATOM(name_, value_) nsIAtom* gfxAtoms::name_ = 0;
#include "gfxAtomList.h"
#undef GFX_ATOM

#define GFX_ATOM(name_, value_) NS_STATIC_ATOM_BUFFER(name_##_buffer, value_)
#include "gfxAtomList.h"
#undef GFX_ATOM

static const nsStaticAtom atoms[] = {
#define GFX_ATOM(name_, value_) NS_STATIC_ATOM(name_##_buffer, &gfxAtoms::name_),
#include "gfxAtomList.h"
#undef GFX_ATOM
};

void gfxAtoms::RegisterAtoms()
{
    NS_RegisterStaticAtoms(atoms, ArrayLength(atoms));
}
