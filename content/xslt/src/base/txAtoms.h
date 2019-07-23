





































#ifndef TRANSFRMX_ATOMS_H
#define TRANSFRMX_ATOMS_H

#ifndef TX_EXE

#include "nsGkAtoms.h"
typedef class nsGkAtoms txXPathAtoms;
typedef class nsGkAtoms txXMLAtoms;
typedef class nsGkAtoms txXSLTAtoms;
typedef class nsGkAtoms txHTMLAtoms;

#else

class nsIAtom;










#define DOM_ATOMS                               \
TX_ATOM(comment, "#comment")                    \
TX_ATOM(document, "#document")                  \
TX_ATOM(text, "#text")

#define XML_ATOMS             \
TX_ATOM(_empty, "")           \
TX_ATOM(base, "base")         \
TX_ATOM(_default, "default")  \
TX_ATOM(lang, "lang")         \
TX_ATOM(preserve, "preserve") \
TX_ATOM(space, "space")       \
TX_ATOM(xml, "xml")           \
TX_ATOM(xmlns, "xmlns")       \
DOM_ATOMS

#define TX_ATOM(_name, _value) static nsIAtom* _name;

class txXMLAtoms
{
public:
    static void init();
XML_ATOMS
};

class txXPathAtoms
{
public:
    static void init();
#include "txXPathAtomList.h"
};

class txXSLTAtoms
{
public:
    static void init();
#include "txXSLTAtomList.h"
};

class txHTMLAtoms
{
public:
    static void init();
#include "txHTMLAtomList.h"
};

#undef TX_ATOM

#endif

#endif
