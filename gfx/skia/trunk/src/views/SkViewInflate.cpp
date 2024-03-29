






#include "SkViewInflate.h"
#include "SkView.h"
#include <stdio.h>

SkViewInflate::SkViewInflate() : fIDs(kMinIDStrAlloc), fStrings(kMinIDStrAlloc)
{
}

SkViewInflate::~SkViewInflate()
{
}

void SkViewInflate::rInflate(const SkDOM& dom, const SkDOM::Node* node, SkView* parent)
{
    const char* str = dom.findAttr(node, "id");
    if (str)
        fIDs.set(str, parent);

    const SkDOM::Node* child = dom.getFirstChild(node);
    while (child)
    {
        SkView* view = this->createView(dom, child);
        if (view)
        {
            this->rInflate(dom, child, view);
            parent->attachChildToFront(view)->unref();
        }
        else
        {
            const char* name = dom.getName(child);
            const char* target;

            if (!strcmp(name, "listenTo") && (target = dom.findAttr(child, "target")) != NULL)
                this->addIDStr(&fListenTo, parent, target);

            if (!strcmp(name, "broadcastTo") && (target = dom.findAttr(child, "target")) != NULL)
                this->addIDStr(&fBroadcastTo, parent, target);
        }
        child = dom.getNextSibling(child);
    }

    parent->setVisibleP(true);
    this->inflateView(parent, dom, node);
}

void SkViewInflate::inflateView(SkView* view, const SkDOM& dom, const SkDOM::Node* node)
{
    
    
    
    view->inflate(dom, node);
}

SkView* SkViewInflate::inflate(const SkDOM& dom, const SkDOM::Node* node, SkView* root)
{
    fIDs.reset();

    if (root == NULL)
    {
        root = this->createView(dom, node);
        if (root == NULL)
        {
            printf("createView returned NULL on <%s>\n", dom.getName(node));
            return NULL;
        }
    }
    this->rInflate(dom, node, root);

    
    {
        SkView*            target;
        const IDStr*    iter = fListenTo.begin();
        const IDStr*    stop = fListenTo.end();
        for (; iter < stop; iter++)
        {
            if (fIDs.find(iter->fStr, &target))
                target->addListenerID(iter->fView->getSinkID());
        }

        iter = fBroadcastTo.begin();
        stop = fBroadcastTo.end();
        for (; iter < stop; iter++)
        {
            if (fIDs.find(iter->fStr, &target))
                iter->fView->addListenerID(target->getSinkID());
        }
    }

    
    root->postInflate(fIDs);
    return root;
}

SkView* SkViewInflate::inflate(const char xml[], size_t len, SkView* root)
{
    SkDOM                dom;
    const SkDOM::Node*    node = dom.build(xml, len);

    return node ? this->inflate(dom, node, root) : NULL;
}

SkView* SkViewInflate::findViewByID(const char id[]) const
{
    SkASSERT(id);
    SkView* view;
    return fIDs.find(id, &view) ? view : NULL;
}

SkView* SkViewInflate::createView(const SkDOM& dom, const SkDOM::Node* node)
{
    if (!strcmp(dom.getName(node), "view"))
        return new SkView;
    return NULL;
}

void SkViewInflate::addIDStr(SkTDArray<IDStr>* list, SkView* view, const char* str)
{
    size_t len = strlen(str) + 1;
    IDStr* pair = list->append();
    pair->fView = view;
    pair->fStr = (char*)fStrings.alloc(len, SkChunkAlloc::kThrow_AllocFailType);
    memcpy(pair->fStr, str, len);
}

#ifdef SK_DEBUG
void SkViewInflate::dump() const
{
    const IDStr* iter = fListenTo.begin();
    const IDStr* stop = fListenTo.end();
    for (; iter < stop; iter++)
        SkDebugf("inflate: listenTo(\"%s\")\n", iter->fStr);

    iter = fBroadcastTo.begin();
    stop = fBroadcastTo.end();
    for (; iter < stop; iter++)
        SkDebugf("inflate: broadcastFrom(\"%s\")\n", iter->fStr);
}
#endif
