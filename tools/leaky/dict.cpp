




































#include <malloc.h>
#include "dict.h"

#ifdef __QNXNTO__

#include <stdio.h>
#endif

MallocDict::MallocDict(int nb)
{
    numBuckets = nb;
    buckets = (MallocDictEntry**) calloc(numBuckets, sizeof(MallocDictEntry*));
    rewind();
}

void MallocDict::rewind(void)
{
    iterNextBucket = -1;
    iterNextEntry = 0;
}

malloc_log_entry* MallocDict::next(void)
{
    if (iterNextEntry) {
	iterNextEntry = iterNextEntry->next;
    }
    while (!iterNextEntry) {
	iterNextBucket++;
	if (iterNextBucket >= numBuckets) {
	    return 0;
	}
	iterNextEntry = buckets[iterNextBucket];
    }
    return iterNextEntry->logEntry;
}

malloc_log_entry** MallocDict::find(u_long addr)
{
    u_long hash = addr % numBuckets;
    MallocDictEntry* mde = buckets[hash];
    while (mde) {
	if (mde->addr == addr) {
	    return &mde->logEntry;
	}
	mde = mde->next;
    }
    return 0;
}

void MallocDict::add(u_long addr, malloc_log_entry *lep)
{
    u_long hash = addr % numBuckets;
    MallocDictEntry** mdep = &buckets[hash];
    MallocDictEntry* mde = new MallocDictEntry;
    mde->addr = addr;
    mde->logEntry = lep;
    mde->next = *mdep;
    *mdep = mde;
}

void MallocDict::remove(u_long addr)
{
    u_long hash = addr % numBuckets;
    MallocDictEntry** mdep = &buckets[hash];
    MallocDictEntry* mde;

    while (NULL != (mde = *mdep)) {
	if (mde->addr == addr) {
	    *mdep = mde->next;

	    return;
	}
	mdep = &mde->next;
    }
}
