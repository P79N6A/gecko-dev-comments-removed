



































#include "gfxAtoms.h"
#include "nsStaticAtom.h"
#include "nsMemory.h"

#define GFX_ATOM(_name, _value) nsIAtom* gfxAtoms::_name = 0;
#include "gfxAtomList.h"
#undef GFX_ATOM

static const nsStaticAtom atoms[] = {
#define GFX_ATOM(_name, _value) { _value, &gfxAtoms::_name },
#include "gfxAtomList.h"
#undef GFX_ATOM
};

void gfxAtoms::RegisterAtoms()
{
    NS_RegisterStaticAtoms(atoms, NS_ARRAY_LENGTH(atoms));
}
