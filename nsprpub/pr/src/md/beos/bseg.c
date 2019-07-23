




































#include "primpl.h"

PR_IMPLEMENT(void)
    _MD_init_segs (void)
{
}

PR_IMPLEMENT(PRStatus)
    _MD_alloc_segment (PRSegment *seg, PRUint32 size, void *vaddr)
{
    return PR_NOT_IMPLEMENTED_ERROR;
}

PR_IMPLEMENT(void)
    _MD_free_segment (PRSegment *seg)
{
}
