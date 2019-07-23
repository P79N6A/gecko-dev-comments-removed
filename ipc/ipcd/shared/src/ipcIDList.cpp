




































#include "ipcIDList.h"

ipcIDNode *
ipcIDList::FindNode(ipcIDNode *node, const nsID &id)
{
    while (node) {
        if (node->Equals(id))
            return node;
        node = node->mNext;
    }
    return NULL;
}

ipcIDNode *
ipcIDList::FindNodeBefore(ipcIDNode *node, const nsID &id)
{
    ipcIDNode *prev = NULL;
    while (node) {
        if (node->Equals(id))
            return prev;
        prev = node;
        node = node->mNext;
    }
    return NULL;
}
