



































#ifndef __JAR_DS_h_
#define __JAR_DS_h_


typedef struct ZZLinkStr ZZLink;
typedef struct ZZListStr ZZList;





struct ZZLinkStr {
    ZZLink *next;
    ZZLink *prev;
    JAR_Item *thing;
};

struct ZZListStr {
    ZZLink link;
};

#define ZZ_InitList(lst)	 \
{ \
    (lst)->link.next = &(lst)->link; \
    (lst)->link.prev = &(lst)->link; \
    (lst)->link.thing = 0; \
}

#define ZZ_ListEmpty(lst) ((lst)->link.next == &(lst)->link)

#define ZZ_ListHead(lst) ((lst)->link.next)

#define ZZ_ListTail(lst) ((lst)->link.prev)

#define ZZ_ListIterDone(lst,lnk) ((lnk) == &(lst)->link)

#define ZZ_AppendLink(lst,lnk)	    \
{ \
    (lnk)->next = &(lst)->link; \
    (lnk)->prev = (lst)->link.prev; \
    (lst)->link.prev->next = (lnk); \
    (lst)->link.prev = (lnk); \
}

#define ZZ_InsertLink(lst,lnk)	    \
{ \
    (lnk)->next = (lst)->link.next; \
    (lnk)->prev = &(lst)->link; \
    (lst)->link.next->prev = (lnk); \
    (lst)->link.next = (lnk); \
}

#define ZZ_RemoveLink(lnk)	 \
{ \
    (lnk)->next->prev = (lnk)->prev; \
    (lnk)->prev->next = (lnk)->next; \
    (lnk)->next = 0; \
    (lnk)->prev = 0; \
}

extern ZZLink *
ZZ_NewLink(JAR_Item *thing);

extern void 
ZZ_DestroyLink(ZZLink *link);

extern ZZList *
ZZ_NewList(void);

extern void 
ZZ_DestroyList(ZZList *list);


#endif 
