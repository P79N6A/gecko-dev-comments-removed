









#include "cmslocal.h"

#include "secerr.h"

















void **
NSS_CMSArray_Alloc(PRArenaPool *poolp, int n)
{
    return (void **)PORT_ArenaZAlloc(poolp, n * sizeof(void *));
}






SECStatus
NSS_CMSArray_Add(PRArenaPool *poolp, void ***array, void *obj)
{
    void **p;
    int n;
    void **dest;

    PORT_Assert(array != NULL);
    if (array == NULL)
	return SECFailure;

    if (*array == NULL) {
	dest = (void **)PORT_ArenaAlloc(poolp, 2 * sizeof(void *));
	n = 0;
    } else {
	n = 0; p = *array;
	while (*p++)
	    n++;
	dest = (void **)PORT_ArenaGrow (poolp, 
			      *array,
			      (n + 1) * sizeof(void *),
			      (n + 2) * sizeof(void *));
    }

    if (dest == NULL)
	return SECFailure;

    dest[n] = obj;
    dest[n+1] = NULL;
    *array = dest;
    return SECSuccess;
}




PRBool
NSS_CMSArray_IsEmpty(void **array)
{
    return (array == NULL || array[0] == NULL);
}




int
NSS_CMSArray_Count(void **array)
{
    int n = 0;

    if (array == NULL)
	return 0;

    while (*array++ != NULL)
	n++;

    return n;
}













void
NSS_CMSArray_Sort(void **primary, int (*compare)(void *,void *), void **secondary, void **tertiary)
{
    int n, i, limit, lastxchg;
    void *tmp;

    n = NSS_CMSArray_Count(primary);

    PORT_Assert(secondary == NULL || NSS_CMSArray_Count(secondary) == n);
    PORT_Assert(tertiary == NULL || NSS_CMSArray_Count(tertiary) == n);
    
    if (n <= 1)	
	return;
    
    
    limit = n - 1;
    while (1) {
	lastxchg = 0;
	for (i = 0; i < limit; i++) {
	    if ((*compare)(primary[i], primary[i+1]) > 0) {
		
		tmp = primary[i+1];
		primary[i+1] = primary[i];
		primary[i] = tmp;
		if (secondary) {		
		    tmp = secondary[i+1];	
		    secondary[i+1] = secondary[i];
		    secondary[i] = tmp;
		}
		if (tertiary) {			
		    tmp = tertiary[i+1];	
		    tertiary[i+1] = tertiary[i];
		    tertiary[i] = tmp;
		}
		lastxchg = i+1;	
	    }
	}
	if (lastxchg == 0)	
	    break;		
	limit = lastxchg;	
    }
}

#if 0



typedef void **NSSCMSArrayIterator;


NSSCMSArrayIterator
NSS_CMSArray_First(void **array)
{
    if (array == NULL || array[0] == NULL)
	return NULL;
    return (NSSCMSArrayIterator)&(array[0]);
}

void *
NSS_CMSArray_Obj(NSSCMSArrayIterator iter)
{
    void **p = (void **)iter;

    return *iter;	
}

NSSCMSArrayIterator
NSS_CMSArray_Next(NSSCMSArrayIterator iter)
{
    void **p = (void **)iter;

    return (NSSCMSArrayIterator)(p + 1);
}

#endif
