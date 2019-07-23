










































#include "primpl.h"







PRStatus _MD_AllocSegment(PRSegment *seg, PRUint32 size, void *vaddr)
{
	PR_ASSERT(seg != 0);
	PR_ASSERT(size != 0);
	PR_ASSERT(vaddr == 0);

	



	seg->vaddr = (char *)malloc(size);

	if (seg->vaddr == NULL) {
		return PR_FAILURE;
	}

	seg->size = size;	

	return PR_SUCCESS;
} 





void _MD_FreeSegment(PRSegment *seg)
{
	PR_ASSERT((seg->flags & _PR_SEG_VM) == 0);

	if (seg->vaddr != NULL)
		free( seg->vaddr );
    return;
} 
