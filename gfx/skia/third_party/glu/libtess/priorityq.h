








































#ifndef __priorityq_sort_h_
#define __priorityq_sort_h_

#include "priorityq-heap.h"

#undef PQkey
#undef PQhandle
#undef PriorityQ
#undef pqNewPriorityQ
#undef pqDeletePriorityQ
#undef pqInit
#undef pqInsert
#undef pqMinimum
#undef pqExtractMin
#undef pqDelete
#undef pqIsEmpty



#define PQkey			PQSortKey
#define PQhandle		PQSortHandle
#define PriorityQ		PriorityQSort

#define pqNewPriorityQ(leq)	__gl_pqSortNewPriorityQ(leq)
#define pqDeletePriorityQ(pq)	__gl_pqSortDeletePriorityQ(pq)














#define pqInit(pq)		__gl_pqSortInit(pq)
#define pqInsert(pq,key)	__gl_pqSortInsert(pq,key)
#define pqMinimum(pq)		__gl_pqSortMinimum(pq)
#define pqExtractMin(pq)	__gl_pqSortExtractMin(pq)
#define pqDelete(pq,handle)	__gl_pqSortDelete(pq,handle)
#define pqIsEmpty(pq)		__gl_pqSortIsEmpty(pq)













typedef PQHeapKey PQkey;
typedef PQHeapHandle PQhandle;
typedef struct PriorityQ PriorityQ;

struct PriorityQ {
  PriorityQHeap	*heap;
  PQkey		*keys;
  PQkey		**order;
  PQhandle	size, max;
  int		initialized;
  int		(*leq)(PQkey key1, PQkey key2);
};
  
PriorityQ	*pqNewPriorityQ( int (*leq)(PQkey key1, PQkey key2) );
void		pqDeletePriorityQ( PriorityQ *pq );

int		pqInit( PriorityQ *pq );
PQhandle	pqInsert( PriorityQ *pq, PQkey key );
PQkey		pqExtractMin( PriorityQ *pq );
void		pqDelete( PriorityQ *pq, PQhandle handle );

PQkey		pqMinimum( PriorityQ *pq );
int		pqIsEmpty( PriorityQ *pq );

#endif
