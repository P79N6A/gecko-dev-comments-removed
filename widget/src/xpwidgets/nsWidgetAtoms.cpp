





































#include "nsWidgetAtoms.h"
#include "nsStaticAtom.h"
#include "nsMemory.h"

#define WIDGET_ATOM(_name, _value) nsIAtom* nsWidgetAtoms::_name = 0;
#include "nsWidgetAtomList.h"
#undef WIDGET_ATOM

#define WIDGET_ATOM(_name, _value) NS_STATIC_ATOM_BUFFER(_name##_buffer, _value)
#include "nsWidgetAtomList.h"
#undef WIDGET_ATOM

static const nsStaticAtom widget_atoms[] = {

#define WIDGET_ATOM(_name, _value) NS_STATIC_ATOM(_name##_buffer, &nsWidgetAtoms::_name),
#include "nsWidgetAtomList.h"
#undef WIDGET_ATOM
};


void nsWidgetAtoms::RegisterAtoms() {

  NS_RegisterStaticAtoms(widget_atoms, NS_ARRAY_LENGTH(widget_atoms));
}
