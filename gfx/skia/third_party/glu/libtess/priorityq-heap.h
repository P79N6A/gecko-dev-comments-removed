








































#ifndef __priorityq_heap_h_
#define __priorityq_heap_h_



#define PQkey			PQHeapKey
#define PQhandle		PQHeapHandle
#define PriorityQ		PriorityQHeap

#define pqNewPriorityQ(leq)	__gl_pqHeapNewPriorityQ(leq)
#define pqDeletePriorityQ(pq)	__gl_pqHeapDeletePriorityQ(pq)














#define pqInit(pq)		__gl_pqHeapInit(pq)
#define pqInsert(pq,key)	__gl_pqHeapInsert(pq,key)
#define pqMinimum(pq)		__gl_pqHeapMinimum(pq)
#define pqExtractMin(pq)	__gl_pqHeapExtractMin(pq)
#define pqDelete(pq,handle)	__gl_pqHeapDelete(pq,handle)
#define pqIsEmpty(pq)		__gl_pqHeapIsEmpty(pq)













typedef void *PQkey;
typedef long PQhandle;
typedef struct PriorityQ PriorityQ;

typedef struct { PQhandle handle; } PQnode;
typedef struct { PQkey key; PQhandle node; } PQhandleElem;

struct PriorityQ {
  PQnode	*nodes;
  PQhandleElem	*handles;
  long		size, max;
  PQhandle	freeList;
  int		initialized;
  int		(*leq)(PQkey key1, PQkey key2);
};
  
PriorityQ	*pqNewPriorityQ( int (*leq)(PQkey key1, PQkey key2) );
void		pqDeletePriorityQ( PriorityQ *pq );

void		pqInit( PriorityQ *pq );
PQhandle	pqInsert( PriorityQ *pq, PQkey key );
PQkey		pqExtractMin( PriorityQ *pq );
void		pqDelete( PriorityQ *pq, PQhandle handle );


#define __gl_pqHeapMinimum(pq)	((pq)->handles[(pq)->nodes[1].handle].key)
#define __gl_pqHeapIsEmpty(pq)	((pq)->size == 0)

#endif
