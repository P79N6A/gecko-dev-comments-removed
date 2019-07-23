




































#include "primpl.h"

#if defined(_PR_PTHREADS)




void _PR_InitSegs(void)
{
}

#else 

void _PR_InitSegs(void)
{
	_PR_MD_INIT_SEGS();
}







PRSegment* _PR_NewSegment(PRUint32 size, void *vaddr)
{
    PRSegment *seg;

	
    seg = PR_NEWZAP(PRSegment);

    if (seg) {
	    size = ((size + _pr_pageSize - 1) >> _pr_pageShift) << _pr_pageShift;
		



	    if (_PR_MD_ALLOC_SEGMENT(seg, size, vaddr) != PR_SUCCESS) {
			PR_DELETE(seg);
			return NULL;
    	}
	}

    return seg;
}




void _PR_DestroySegment(PRSegment *seg)
{
	_PR_MD_FREE_SEGMENT(seg);
    PR_DELETE(seg);
}

#endif 
