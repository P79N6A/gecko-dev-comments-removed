



































#include "jar.h"


ZZList *
ZZ_NewList(void)
{
    ZZList *list = (ZZList *) PORT_ZAlloc (sizeof (ZZList));
    if (list)
	ZZ_InitList (list);
    return list;
}

ZZLink *
ZZ_NewLink(JAR_Item *thing)
{
    ZZLink *link = (ZZLink *) PORT_ZAlloc (sizeof (ZZLink));
    if (link)
	link->thing = thing;
    return link;
}

void 
ZZ_DestroyLink(ZZLink *link)
{
    PORT_Free(link);
}

void 
ZZ_DestroyList (ZZList *list)
{
    PORT_Free(list);
}
