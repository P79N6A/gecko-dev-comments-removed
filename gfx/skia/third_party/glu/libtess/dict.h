








































#ifndef __dict_list_h_
#define __dict_list_h_

#include <sk_glu.h>



#define DictKey		DictListKey
#define Dict		DictList
#define DictNode	DictListNode

#define dictNewDict(frame,leq)		__gl_dictListNewDict(frame,leq)
#define dictDeleteDict(dict)		__gl_dictListDeleteDict(dict)

#define dictSearch(dict,key)		__gl_dictListSearch(dict,key)
#define dictInsert(dict,key)		__gl_dictListInsert(dict,key)
#define dictInsertBefore(dict,node,key)	__gl_dictListInsertBefore(dict,node,key)
#define dictDelete(dict,node)		__gl_dictListDelete(dict,node)

#define dictKey(n)			__gl_dictListKey(n)
#define dictSucc(n)			__gl_dictListSucc(n)
#define dictPred(n)			__gl_dictListPred(n)
#define dictMin(d)			__gl_dictListMin(d)
#define dictMax(d)			__gl_dictListMax(d)



typedef void *DictKey;
typedef struct Dict Dict;
typedef struct DictNode DictNode;

Dict		*dictNewDict(
			void *frame,
			int (*leq)(void *frame, DictKey key1, DictKey key2) );
			
void		dictDeleteDict( Dict *dict );





DictNode	*dictSearch( Dict *dict, DictKey key );
DictNode	*dictInsertBefore( Dict *dict, DictNode *node, DictKey key );
void		dictDelete( Dict *dict, DictNode *node );

#define		__gl_dictListKey(n)	((n)->key)
#define		__gl_dictListSucc(n)	((n)->next)
#define		__gl_dictListPred(n)	((n)->prev)
#define		__gl_dictListMin(d)	((d)->head.next)
#define		__gl_dictListMax(d)	((d)->head.prev)
#define	       __gl_dictListInsert(d,k) (dictInsertBefore((d),&(d)->head,(k)))




struct DictNode {
  DictKey	key;
  DictNode	*next;
  DictNode	*prev;
};

struct Dict {
  DictNode	head;
  void		*frame;
  int		(*leq)(void *frame, DictKey key1, DictKey key2);
};

#endif
