#define	JEMALLOC_BITMAP_C_
#include "jemalloc/internal/jemalloc_internal.h"




static size_t	bits2groups(size_t nbits);



static size_t
bits2groups(size_t nbits)
{

	return ((nbits >> LG_BITMAP_GROUP_NBITS) +
	    !!(nbits & BITMAP_GROUP_NBITS_MASK));
}

void
bitmap_info_init(bitmap_info_t *binfo, size_t nbits)
{
	unsigned i;
	size_t group_count;

	assert(nbits > 0);
	assert(nbits <= (ZU(1) << LG_BITMAP_MAXBITS));

	




	binfo->levels[0].group_offset = 0;
	group_count = bits2groups(nbits);
	for (i = 1; group_count > 1; i++) {
		assert(i < BITMAP_MAX_LEVELS);
		binfo->levels[i].group_offset = binfo->levels[i-1].group_offset
		    + group_count;
		group_count = bits2groups(group_count);
	}
	binfo->levels[i].group_offset = binfo->levels[i-1].group_offset
	    + group_count;
	binfo->nlevels = i;
	binfo->nbits = nbits;
}

size_t
bitmap_info_ngroups(const bitmap_info_t *binfo)
{

	return (binfo->levels[binfo->nlevels].group_offset << LG_SIZEOF_BITMAP);
}

size_t
bitmap_size(size_t nbits)
{
	bitmap_info_t binfo;

	bitmap_info_init(&binfo, nbits);
	return (bitmap_info_ngroups(&binfo));
}

void
bitmap_init(bitmap_t *bitmap, const bitmap_info_t *binfo)
{
	size_t extra;
	unsigned i;

	






	memset(bitmap, 0xffU, binfo->levels[binfo->nlevels].group_offset <<
	    LG_SIZEOF_BITMAP);
	extra = (BITMAP_GROUP_NBITS - (binfo->nbits & BITMAP_GROUP_NBITS_MASK))
	    & BITMAP_GROUP_NBITS_MASK;
	if (extra != 0)
		bitmap[binfo->levels[1].group_offset - 1] >>= extra;
	for (i = 1; i < binfo->nlevels; i++) {
		size_t group_count = binfo->levels[i].group_offset -
		    binfo->levels[i-1].group_offset;
		extra = (BITMAP_GROUP_NBITS - (group_count &
		    BITMAP_GROUP_NBITS_MASK)) & BITMAP_GROUP_NBITS_MASK;
		if (extra != 0)
			bitmap[binfo->levels[i+1].group_offset - 1] >>= extra;
	}
}
