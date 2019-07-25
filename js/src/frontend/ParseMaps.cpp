







































#include "ParseMaps-inl.h"

using namespace js;

void
ParseMapPool::checkInvariants()
{
    



    JS_STATIC_ASSERT(sizeof(JSDefinition *) == sizeof(jsatomid));
    JS_STATIC_ASSERT(sizeof(JSDefinition *) == sizeof(DefnOrHeader));
    JS_STATIC_ASSERT(sizeof(AtomDefnMap::Entry) == sizeof(AtomIndexMap::Entry));
    JS_STATIC_ASSERT(sizeof(AtomDefnMap::Entry) == sizeof(AtomDOHMap::Entry));
    JS_STATIC_ASSERT(sizeof(AtomMapT::Entry) == sizeof(AtomDOHMap::Entry));
    
    JS_STATIC_ASSERT(tl::IsPodType<AtomIndexMap::WordMap::Entry>::result);
    JS_STATIC_ASSERT(tl::IsPodType<AtomDOHMap::WordMap::Entry>::result);
    JS_STATIC_ASSERT(tl::IsPodType<AtomDefnMap::WordMap::Entry>::result);
}

void
ParseMapPool::purgeAll()
{
    for (void **it = all.begin(), **end = all.end(); it != end; ++it)
        cx->delete_<AtomMapT>(asAtomMap(*it));

    all.clearAndFree();
    recyclable.clearAndFree();
}

void *
ParseMapPool::allocateFresh()
{
    size_t newAllLength = all.length() + 1;
    if (!all.reserve(newAllLength) || !recyclable.reserve(newAllLength))
        return NULL;

    AtomMapT *map = cx->new_<AtomMapT>(cx);
    if (!map)
        return NULL;

    all.infallibleAppend(map);
    return (void *) map;
}

#ifdef DEBUG
void
AtomDecls::dump()
{
    for (AtomDOHRange r = map->all(); !r.empty(); r.popFront()) {
        fprintf(stderr, "atom: ");
        js_DumpAtom(r.front().key());
        const DefnOrHeader &doh = r.front().value();
        if (doh.isHeader()) {
            AtomDeclNode *node = doh.header();
            do {
                fprintf(stderr, "  node: %p\n", (void *) node);
                fprintf(stderr, "    defn: %p\n", (void *) node->defn);
                node = node->next;
            } while (node);
        } else {
            fprintf(stderr, "  defn: %p\n", (void *) doh.defn());
        }
    }
}

void
DumpAtomDefnMap(const AtomDefnMapPtr &map)
{
    if (map->empty()) {
        fprintf(stderr, "empty\n");
        return;
    }

    for (AtomDefnRange r = map->all(); !r.empty(); r.popFront()) {
        fprintf(stderr, "atom: ");
        js_DumpAtom(r.front().key());
        fprintf(stderr, "defn: %p\n", (void *) r.front().value());
    }
}
#endif

AtomDeclNode *
AtomDecls::allocNode(JSDefinition *defn)
{
    AtomDeclNode *p;
    JS_ARENA_ALLOCATE_TYPE(p, AtomDeclNode, &cx->tempPool);
    if (!p) {
        js_ReportOutOfMemory(cx);
        return NULL;
    }
    return new (p) AtomDeclNode(defn);
}

bool
AtomDecls::addShadow(JSAtom *atom, JSDefinition *defn)
{
    AtomDeclNode *node = allocNode(defn);
    if (!node)
        return false;

    AtomDOHAddPtr p = map->lookupForAdd(atom);
    if (!p)
        return map->add(p, atom, DefnOrHeader(node));

    AtomDeclNode *toShadow;
    if (p.value().isHeader()) {
        toShadow = p.value().header();
    } else {
        toShadow = allocNode(p.value().defn());
        if (!toShadow)
            return false;
    }
    node->next = toShadow;
    p.value() = DefnOrHeader(node);
    return true;
}

AtomDeclNode *
AtomDecls::lastAsNode(DefnOrHeader *doh)
{
    if (doh->isHeader()) {
        AtomDeclNode *last = doh->header();
        while (last->next)
            last = last->next;
        return last;
    }

    
    AtomDeclNode *node = allocNode(doh->defn());
    if (!node)
        return NULL;
    *doh = DefnOrHeader(node);
    return node;
}

bool
AtomDecls::addHoist(JSAtom *atom, JSDefinition *defn)
{
    AtomDeclNode *node = allocNode(defn);
    if (!node)
        return false;

    AtomDOHAddPtr p = map->lookupForAdd(atom);
    if (p) {
        AtomDeclNode *last = lastAsNode(&p.value());
        if (!last)
            return false;
        last->next = node;
        return true;
    }

    return map->add(p, atom, DefnOrHeader(node));
}
