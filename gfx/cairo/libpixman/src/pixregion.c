














































#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "pixregionint.h"

#if defined (__GNUC__) && !defined (NO_INLINES)
#define INLINE	__inline
#else
#define INLINE
#endif

#undef assert
#ifdef DEBUG_PIXREGION
#define assert(expr) {if (!(expr)) \
		FatalError("Assertion failed file %s, line %d: expr\n", \
			__FILE__, __LINE__); }
#else
#define assert(expr)
#endif

#define good(reg) assert(pixman_region16_valid(reg))

#undef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))

static pixman_box16_t pixman_region_emptyBox = {0, 0, 0, 0};
static pixman_region16_data_t pixman_region_emptyData = {0, 0};

static pixman_region16_data_t  pixman_brokendata = {0, 0};
static pixman_region16_t   pixman_brokenregion = { { 0, 0, 0, 0 }, &pixman_brokendata };

static pixman_region_status_t
pixman_break (pixman_region16_t *pReg);

static void
pixman_init (pixman_region16_t *region, pixman_box16_t *rect);

static void
pixman_uninit (pixman_region16_t *region);


















































#define EXTENTCHECK(r1,r2) \
      (!( ((r1)->x2 <= (r2)->x1)  || \
          ((r1)->x1 >= (r2)->x2)  || \
          ((r1)->y2 <= (r2)->y1)  || \
          ((r1)->y1 >= (r2)->y2) ) )


#define INBOX(r,x,y) \
      ( ((r)->x2 >  x) && \
        ((r)->x1 <= x) && \
        ((r)->y2 >  y) && \
        ((r)->y1 <= y) )


#define SUBSUMES(r1,r2) \
      ( ((r1)->x1 <= (r2)->x1) && \
        ((r1)->x2 >= (r2)->x2) && \
        ((r1)->y1 <= (r2)->y1) && \
        ((r1)->y2 >= (r2)->y2) )

#define allocData(n) malloc(PIXREGION_SZOF(n))
#define freeData(reg) if ((reg)->data && (reg)->data->size) free((reg)->data)

#define RECTALLOC_BAIL(pReg,n,bail) \
if (!(pReg)->data || (((pReg)->data->numRects + (n)) > (pReg)->data->size)) \
    if (!pixman_rect_alloc(pReg, n)) { goto bail; }

#define RECTALLOC(pReg,n) \
if (!(pReg)->data || (((pReg)->data->numRects + (n)) > (pReg)->data->size)) \
    if (!pixman_rect_alloc(pReg, n)) { return PIXMAN_REGION_STATUS_FAILURE; }

#define ADDRECT(pNextRect,nx1,ny1,nx2,ny2)	\
{						\
    pNextRect->x1 = nx1;			\
    pNextRect->y1 = ny1;			\
    pNextRect->x2 = nx2;			\
    pNextRect->y2 = ny2;			\
    pNextRect++;				\
}

#define NEWRECT(pReg,pNextRect,nx1,ny1,nx2,ny2)			\
{									\
    if (!(pReg)->data || ((pReg)->data->numRects == (pReg)->data->size))\
    {									\
	if (!pixman_rect_alloc(pReg, 1))					\
	    return PIXMAN_REGION_STATUS_FAILURE;						\
	pNextRect = PIXREGION_TOP(pReg);					\
    }									\
    ADDRECT(pNextRect,nx1,ny1,nx2,ny2);					\
    pReg->data->numRects++;						\
    assert(pReg->data->numRects<=pReg->data->size);			\
}

#define DOWNSIZE(reg,numRects)						 \
if (((numRects) < ((reg)->data->size >> 1)) && ((reg)->data->size > 50)) \
{									 \
    pixman_region16_data_t * NewData;							 \
    NewData = (pixman_region16_data_t *)realloc((reg)->data, PIXREGION_SZOF(numRects));	 \
    if (NewData)							 \
    {									 \
	NewData->size = (numRects);					 \
	(reg)->data = NewData;						 \
    }									 \
}

#ifdef DEBUG_PIXREGION
int
pixman_region16_print(rgn)
    pixman_region16_t * rgn;
{
    int num, size;
    int i;
    pixman_box16_t * rects;

    num = PIXREGION_NUM_RECTS(rgn);
    size = PIXREGION_SIZE(rgn);
    rects = PIXREGION_RECTS(rgn);
    ErrorF("num: %d size: %d\n", num, size);
    ErrorF("extents: %d %d %d %d\n",
	   rgn->extents.x1, rgn->extents.y1, rgn->extents.x2, rgn->extents.y2);
    for (i = 0; i < num; i++)
      ErrorF("%d %d %d %d \n",
	     rects[i].x1, rects[i].y1, rects[i].x2, rects[i].y2);
    ErrorF("\n");
    return(num);
}

pixman_region_status_t
pixman_region16_tsEqual(reg1, reg2)
    pixman_region16_t * reg1;
    pixman_region16_t * reg2;
{
    int i;
    pixman_box16_t * rects1, rects2;

    if (reg1->extents.x1 != reg2->extents.x1) return PIXMAN_REGION_STATUS_FAILURE;
    if (reg1->extents.x2 != reg2->extents.x2) return PIXMAN_REGION_STATUS_FAILURE;
    if (reg1->extents.y1 != reg2->extents.y1) return PIXMAN_REGION_STATUS_FAILURE;
    if (reg1->extents.y2 != reg2->extents.y2) return PIXMAN_REGION_STATUS_FAILURE;
    if (PIXREGION_NUM_RECTS(reg1) != PIXREGION_NUM_RECTS(reg2)) return PIXMAN_REGION_STATUS_FAILURE;

    rects1 = PIXREGION_RECTS(reg1);
    rects2 = PIXREGION_RECTS(reg2);
    for (i = 0; i != PIXREGION_NUM_RECTS(reg1); i++) {
	if (rects1[i].x1 != rects2[i].x1) return PIXMAN_REGION_STATUS_FAILURE;
	if (rects1[i].x2 != rects2[i].x2) return PIXMAN_REGION_STATUS_FAILURE;
	if (rects1[i].y1 != rects2[i].y1) return PIXMAN_REGION_STATUS_FAILURE;
	if (rects1[i].y2 != rects2[i].y2) return PIXMAN_REGION_STATUS_FAILURE;
    }
    return PIXMAN_REGION_STATUS_SUCCESS;
}

pixman_region_status_t
pixman_region16_valid(reg)
    pixman_region16_t * reg;
{
    int i, numRects;

    if ((reg->extents.x1 > reg->extents.x2) ||
	(reg->extents.y1 > reg->extents.y2))
	return PIXMAN_REGION_STATUS_FAILURE;
    numRects = PIXREGION_NUM_RECTS(reg);
    if (!numRects)
	return ((reg->extents.x1 == reg->extents.x2) &&
		(reg->extents.y1 == reg->extents.y2) &&
		(reg->data->size || (reg->data == &pixman_region_emptyData)));
    else if (numRects == 1)
	return (!reg->data);
    else
    {
	pixman_box16_t * pboxP, pboxN;
	pixman_box16_t box;

	pboxP = PIXREGION_RECTS(reg);
	box = *pboxP;
	box.y2 = pboxP[numRects-1].y2;
	pboxN = pboxP + 1;
	for (i = numRects; --i > 0; pboxP++, pboxN++)
	{
	    if ((pboxN->x1 >= pboxN->x2) ||
		(pboxN->y1 >= pboxN->y2))
		return PIXMAN_REGION_STATUS_FAILURE;
	    if (pboxN->x1 < box.x1)
	        box.x1 = pboxN->x1;
	    if (pboxN->x2 > box.x2)
		box.x2 = pboxN->x2;
	    if ((pboxN->y1 < pboxP->y1) ||
		((pboxN->y1 == pboxP->y1) &&
		 ((pboxN->x1 < pboxP->x2) || (pboxN->y2 != pboxP->y2))))
		return PIXMAN_REGION_STATUS_FAILURE;
	}
	return ((box.x1 == reg->extents.x1) &&
		(box.x2 == reg->extents.x2) &&
		(box.y1 == reg->extents.y1) &&
		(box.y2 == reg->extents.y2));
    }
}

#endif 


pixman_region16_t *
pixman_region_create (void)
{
    return pixman_region_create_simple (NULL);
}






pixman_region16_t *
pixman_region_create_simple (pixman_box16_t *extents)
{
    pixman_region16_t *region;

    region = malloc (sizeof (pixman_region16_t));
    if (region == NULL)
	return &pixman_brokenregion;

    pixman_init (region, extents);

    return region;
}






static void
pixman_init(pixman_region16_t *region, pixman_box16_t *extents)
{
    if (extents)
    {
	region->extents = *extents;
	region->data = NULL;
    }
    else
    {
	region->extents = pixman_region_emptyBox;
	region->data = &pixman_region_emptyData;
    }
}

static void
pixman_uninit (pixman_region16_t *region)
{
    good (region);
    freeData (region);
}

void
pixman_region_destroy (pixman_region16_t *region)
{
    pixman_uninit (region);

    if (region != &pixman_brokenregion)
	free (region);
}

int
pixman_region_num_rects (pixman_region16_t *region)
{
    return PIXREGION_NUM_RECTS (region);
}

pixman_box16_t *
pixman_region_rects (pixman_region16_t *region)
{
    return PIXREGION_RECTS (region);
}

static pixman_region_status_t
pixman_break (pixman_region16_t *region)
{
    freeData (region);
    region->extents = pixman_region_emptyBox;
    region->data = &pixman_brokendata;
    return PIXMAN_REGION_STATUS_FAILURE;
}

static pixman_region_status_t
pixman_rect_alloc(pixman_region16_t * region, int n)
{
    pixman_region16_data_t *data;

    if (!region->data)
    {
	n++;
	region->data = allocData(n);
	if (!region->data)
	    return pixman_break (region);
	region->data->numRects = 1;
	*PIXREGION_BOXPTR(region) = region->extents;
    }
    else if (!region->data->size)
    {
	region->data = allocData(n);
	if (!region->data)
	    return pixman_break (region);
	region->data->numRects = 0;
    }
    else
    {
	if (n == 1)
	{
	    n = region->data->numRects;
	    if (n > 500) 
		n = 250;
	}
	n += region->data->numRects;
	data = (pixman_region16_data_t *)realloc(region->data, PIXREGION_SZOF(n));
	if (!data)
	    return pixman_break (region);
	region->data = data;
    }
    region->data->size = n;
    return PIXMAN_REGION_STATUS_SUCCESS;
}

pixman_region_status_t
pixman_region_copy(pixman_region16_t *dst, pixman_region16_t *src)
{
    good(dst);
    good(src);
    if (dst == src)
	return PIXMAN_REGION_STATUS_SUCCESS;
    dst->extents = src->extents;
    if (!src->data || !src->data->size)
    {
	freeData(dst);
	dst->data = src->data;
	return PIXMAN_REGION_STATUS_SUCCESS;
    }
    if (!dst->data || (dst->data->size < src->data->numRects))
    {
	freeData(dst);
	dst->data = allocData(src->data->numRects);
	if (!dst->data)
	    return pixman_break (dst);
	dst->data->size = src->data->numRects;
    }
    dst->data->numRects = src->data->numRects;
    memmove((char *)PIXREGION_BOXPTR(dst),(char *)PIXREGION_BOXPTR(src),
	  dst->data->numRects * sizeof(pixman_box16_t));
    return PIXMAN_REGION_STATUS_SUCCESS;
}























INLINE static int
pixman_coalesce (
    pixman_region16_t *	region,	    	
    int	    	  	prevStart,  	
    int	    	  	curStart)   	
{
    pixman_box16_t *	pPrevBox;   	
    pixman_box16_t *	pCurBox;    	
    int  	numRects;	
    int	y2;		
    


    numRects = curStart - prevStart;
    assert(numRects == region->data->numRects - curStart);

    if (!numRects) return curStart;

    



    pPrevBox = PIXREGION_BOX(region, prevStart);
    pCurBox = PIXREGION_BOX(region, curStart);
    if (pPrevBox->y2 != pCurBox->y1) return curStart;

    





    y2 = pCurBox->y2;

    do {
	if ((pPrevBox->x1 != pCurBox->x1) || (pPrevBox->x2 != pCurBox->x2)) {
	    return (curStart);
	}
	pPrevBox++;
	pCurBox++;
	numRects--;
    } while (numRects);

    



    numRects = curStart - prevStart;
    region->data->numRects -= numRects;
    do {
	pPrevBox--;
	pPrevBox->y2 = y2;
	numRects--;
    } while (numRects);
    return prevStart;
}



#define Coalesce(newReg, prevBand, curBand)				\
    if (curBand - prevBand == newReg->data->numRects - curBand) {	\
	prevBand = pixman_coalesce(newReg, prevBand, curBand);		\
    } else {								\
	prevBand = curBand;						\
    }


















INLINE static pixman_region_status_t
pixman_region_appendNonO (
    pixman_region16_t *	region,
    pixman_box16_t *	r,
    pixman_box16_t *  	  	rEnd,
    int  	y1,
    int  	y2)
{
    pixman_box16_t *	pNextRect;
    int	newRects;

    newRects = rEnd - r;

    assert(y1 < y2);
    assert(newRects != 0);

    
    RECTALLOC(region, newRects);
    pNextRect = PIXREGION_TOP(region);
    region->data->numRects += newRects;
    do {
	assert(r->x1 < r->x2);
	ADDRECT(pNextRect, r->x1, y1, r->x2, y2);
	r++;
    } while (r != rEnd);

    return PIXMAN_REGION_STATUS_SUCCESS;
}

#define FindBand(r, rBandEnd, rEnd, ry1)		    \
{							    \
    ry1 = r->y1;					    \
    rBandEnd = r+1;					    \
    while ((rBandEnd != rEnd) && (rBandEnd->y1 == ry1)) {   \
	rBandEnd++;					    \
    }							    \
}

#define	AppendRegions(newReg, r, rEnd)					\
{									\
    int newRects;							\
    if ((newRects = rEnd - r)) {					\
	RECTALLOC(newReg, newRects);					\
	memmove((char *)PIXREGION_TOP(newReg),(char *)r, 			\
              newRects * sizeof(pixman_box16_t));				\
	newReg->data->numRects += newRects;				\
    }									\
}






























typedef pixman_region_status_t (*OverlapProcPtr)(
    pixman_region16_t	 *region,
    pixman_box16_t *r1,
    pixman_box16_t *r1End,
    pixman_box16_t *r2,
    pixman_box16_t *r2End,
    short    	 y1,
    short    	 y2,
    int		 *pOverlap);

static pixman_region_status_t
pixman_op(
    pixman_region16_t *       newReg,		    
    pixman_region16_t *       reg1,		    
    pixman_region16_t *       reg2,		    
    OverlapProcPtr  overlapFunc,            

    int	    appendNon1,		    
					    
    int	    appendNon2,		    
					    
    int	    *pOverlap)
{
    pixman_box16_t * r1;			    
    pixman_box16_t * r2;			    
    pixman_box16_t *	    r1End;		    
    pixman_box16_t *	    r2End;		    
    short	    ybot;		    
    short	    ytop;		    
    pixman_region16_data_t *	    oldData;		    
    int		    prevBand;		    

    int		    curBand;		    

    pixman_box16_t * r1BandEnd;		    
    pixman_box16_t * r2BandEnd;		    
    short	    top;		    
    short	    bot;		    
    int    r1y1;		    
    int    r2y1;
    int		    newSize;
    int		    numRects;

    


    if (PIXREGION_NAR (reg1) || PIXREGION_NAR(reg2))
	return pixman_break (newReg);

    







    r1 = PIXREGION_RECTS(reg1);
    newSize = PIXREGION_NUM_RECTS(reg1);
    r1End = r1 + newSize;
    numRects = PIXREGION_NUM_RECTS(reg2);
    r2 = PIXREGION_RECTS(reg2);
    r2End = r2 + numRects;
    assert(r1 != r1End);
    assert(r2 != r2End);

    oldData = (pixman_region16_data_t *)NULL;
    if (((newReg == reg1) && (newSize > 1)) ||
	((newReg == reg2) && (numRects > 1)))
    {
	oldData = newReg->data;
	newReg->data = &pixman_region_emptyData;
    }
    
    if (numRects > newSize)
	newSize = numRects;
    newSize <<= 1;
    if (!newReg->data)
	newReg->data = &pixman_region_emptyData;
    else if (newReg->data->size)
	newReg->data->numRects = 0;
    if (newSize > newReg->data->size)
	if (!pixman_rect_alloc(newReg, newSize))
	    return PIXMAN_REGION_STATUS_FAILURE;

    













    ybot = MIN(r1->y1, r2->y1);

    








    prevBand = 0;

    do {
	






	assert(r1 != r1End);
	assert(r2 != r2End);

	FindBand(r1, r1BandEnd, r1End, r1y1);
	FindBand(r2, r2BandEnd, r2End, r2y1);

	







	if (r1y1 < r2y1) {
	    if (appendNon1) {
		top = MAX(r1y1, ybot);
		bot = MIN(r1->y2, r2y1);
		if (top != bot)	{
		    curBand = newReg->data->numRects;
		    pixman_region_appendNonO(newReg, r1, r1BandEnd, top, bot);
		    Coalesce(newReg, prevBand, curBand);
		}
	    }
	    ytop = r2y1;
	} else if (r2y1 < r1y1) {
	    if (appendNon2) {
		top = MAX(r2y1, ybot);
		bot = MIN(r2->y2, r1y1);
		if (top != bot) {
		    curBand = newReg->data->numRects;
		    pixman_region_appendNonO(newReg, r2, r2BandEnd, top, bot);
		    Coalesce(newReg, prevBand, curBand);
		}
	    }
	    ytop = r1y1;
	} else {
	    ytop = r1y1;
	}

	



	ybot = MIN(r1->y2, r2->y2);
	if (ybot > ytop) {
	    curBand = newReg->data->numRects;
	    (* overlapFunc)(newReg, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot,
			    pOverlap);
	    Coalesce(newReg, prevBand, curBand);
	}

	



	if (r1->y2 == ybot) r1 = r1BandEnd;
	if (r2->y2 == ybot) r2 = r2BandEnd;

    } while (r1 != r1End && r2 != r2End);

    







    if ((r1 != r1End) && appendNon1) {
	
	FindBand(r1, r1BandEnd, r1End, r1y1);
	curBand = newReg->data->numRects;
	pixman_region_appendNonO(newReg, r1, r1BandEnd, MAX(r1y1, ybot), r1->y2);
	Coalesce(newReg, prevBand, curBand);
	
	AppendRegions(newReg, r1BandEnd, r1End);

    } else if ((r2 != r2End) && appendNon2) {
	
	FindBand(r2, r2BandEnd, r2End, r2y1);
	curBand = newReg->data->numRects;
	pixman_region_appendNonO(newReg, r2, r2BandEnd, MAX(r2y1, ybot), r2->y2);
	Coalesce(newReg, prevBand, curBand);
	
	AppendRegions(newReg, r2BandEnd, r2End);
    }

    if (oldData)
	free(oldData);

    if (!(numRects = newReg->data->numRects))
    {
	freeData(newReg);
	newReg->data = &pixman_region_emptyData;
    }
    else if (numRects == 1)
    {
	newReg->extents = *PIXREGION_BOXPTR(newReg);
	freeData(newReg);
	newReg->data = (pixman_region16_data_t *)NULL;
    }
    else
    {
	DOWNSIZE(newReg, numRects);
    }

    return PIXMAN_REGION_STATUS_SUCCESS;
}
















static void
pixman_set_extents (pixman_region16_t *region)
{
    pixman_box16_t *box, *boxEnd;

    if (!region->data)
	return;
    if (!region->data->size)
    {
	region->extents.x2 = region->extents.x1;
	region->extents.y2 = region->extents.y1;
	return;
    }

    box = PIXREGION_BOXPTR(region);
    boxEnd = PIXREGION_END(region);

    






    region->extents.x1 = box->x1;
    region->extents.y1 = box->y1;
    region->extents.x2 = boxEnd->x2;
    region->extents.y2 = boxEnd->y2;

    assert(region->extents.y1 < region->extents.y2);
    while (box <= boxEnd) {
	if (box->x1 < region->extents.x1)
	    region->extents.x1 = box->x1;
	if (box->x2 > region->extents.x2)
	    region->extents.x2 = box->x2;
	box++;
    };

    assert(region->extents.x1 < region->extents.x2);
}


















static pixman_region_status_t
pixman_region_intersectO (
    pixman_region16_t *	region,
    pixman_box16_t *	r1,
    pixman_box16_t *  	r1End,
    pixman_box16_t *	r2,
    pixman_box16_t *  	r2End,
    short    	  	y1,
    short    	  	y2,
    int		*pOverlap)
{
    int  	x1;
    int  	x2;
    pixman_box16_t *	pNextRect;

    pNextRect = PIXREGION_TOP(region);

    assert(y1 < y2);
    assert(r1 != r1End && r2 != r2End);

    do {
	x1 = MAX(r1->x1, r2->x1);
	x2 = MIN(r1->x2, r2->x2);

	



	if (x1 < x2)
	    NEWRECT(region, pNextRect, x1, y1, x2, y2);

	




	if (r1->x2 == x2) {
	    r1++;
	}
	if (r2->x2 == x2) {
	    r2++;
	}
    } while ((r1 != r1End) && (r2 != r2End));

    return PIXMAN_REGION_STATUS_SUCCESS;
}

pixman_region_status_t
pixman_region_intersect(pixman_region16_t * 	newReg,
			pixman_region16_t * 	reg1,
			pixman_region16_t *	reg2)
{
    good(reg1);
    good(reg2);
    good(newReg);
   
    if (PIXREGION_NIL(reg1)  || PIXREGION_NIL(reg2) ||
	!EXTENTCHECK(&reg1->extents, &reg2->extents))
    {
	
	freeData(newReg);
	newReg->extents.x2 = newReg->extents.x1;
	newReg->extents.y2 = newReg->extents.y1;
	if (PIXREGION_NAR(reg1) || PIXREGION_NAR(reg2))
	{
	    newReg->data = &pixman_brokendata;
	    return PIXMAN_REGION_STATUS_FAILURE;
	}
	else
	    newReg->data = &pixman_region_emptyData;
    }
    else if (!reg1->data && !reg2->data)
    {
	
	newReg->extents.x1 = MAX(reg1->extents.x1, reg2->extents.x1);
	newReg->extents.y1 = MAX(reg1->extents.y1, reg2->extents.y1);
	newReg->extents.x2 = MIN(reg1->extents.x2, reg2->extents.x2);
	newReg->extents.y2 = MIN(reg1->extents.y2, reg2->extents.y2);
	freeData(newReg);
	newReg->data = (pixman_region16_data_t *)NULL;
    }
    else if (!reg2->data && SUBSUMES(&reg2->extents, &reg1->extents))
    {
	return pixman_region_copy(newReg, reg1);
    }
    else if (!reg1->data && SUBSUMES(&reg1->extents, &reg2->extents))
    {
	return pixman_region_copy(newReg, reg2);
    }
    else if (reg1 == reg2)
    {
	return pixman_region_copy(newReg, reg1);
    }
    else
    {
	
	int overlap; 
	if (!pixman_op(newReg, reg1, reg2, pixman_region_intersectO, PIXMAN_REGION_STATUS_FAILURE, PIXMAN_REGION_STATUS_FAILURE,
			&overlap))
	    return PIXMAN_REGION_STATUS_FAILURE;
	pixman_set_extents(newReg);
    }

    good(newReg);
    return(PIXMAN_REGION_STATUS_SUCCESS);
}

#define MERGERECT(r)						\
{								\
    if (r->x1 <= x2) {						\
	/* Merge with current rectangle */			\
	if (r->x1 < x2) *pOverlap = PIXMAN_REGION_STATUS_SUCCESS;				\
	if (x2 < r->x2) x2 = r->x2;				\
    } else {							\
	/* Add current rectangle, start new one */		\
	NEWRECT(region, pNextRect, x1, y1, x2, y2);		\
	x1 = r->x1;						\
	x2 = r->x2;						\
    }								\
    r++;							\
}




















static pixman_region_status_t
pixman_region_unionO (
    pixman_region16_t	 *region,
    pixman_box16_t *r1,
    pixman_box16_t *r1End,
    pixman_box16_t *r2,
    pixman_box16_t *r2End,
    short	  y1,
    short	  y2,
    int		  *pOverlap)
{
    pixman_box16_t *     pNextRect;
    int        x1;     
    int        x2;

    assert (y1 < y2);
    assert(r1 != r1End && r2 != r2End);

    pNextRect = PIXREGION_TOP(region);

    
    if (r1->x1 < r2->x1)
    {
	x1 = r1->x1;
	x2 = r1->x2;
	r1++;
    }
    else
    {
	x1 = r2->x1;
	x2 = r2->x2;
	r2++;
    }
    while (r1 != r1End && r2 != r2End)
    {
	if (r1->x1 < r2->x1) MERGERECT(r1) else MERGERECT(r2);
    }

    
    if (r1 != r1End)
    {
	do
	{
	    MERGERECT(r1);
	} while (r1 != r1End);
    }
    else if (r2 != r2End)
    {
	do
	{
	    MERGERECT(r2);
	} while (r2 != r2End);
    }

    
    NEWRECT(region, pNextRect, x1, y1, x2, y2);

    return PIXMAN_REGION_STATUS_SUCCESS;
}


pixman_region_status_t
pixman_region_union_rect(pixman_region16_t *dest, pixman_region16_t *source,
		   int x, int y, unsigned int width, unsigned int height)
{
    pixman_region16_t region;

    if (!width || !height)
	return pixman_region_copy (dest, source);
    region.data = NULL;
    region.extents.x1 = x;
    region.extents.y1 = y;
    region.extents.x2 = x + width;
    region.extents.y2 = y + height;

    return pixman_region_union (dest, source, &region);
}

pixman_region_status_t
pixman_region_union(pixman_region16_t *newReg, pixman_region16_t *reg1, pixman_region16_t *reg2)
{
    int overlap; 

    
    good(reg1);
    good(reg2);
    good(newReg);
    

    


    if (reg1 == reg2)
    {
	return pixman_region_copy(newReg, reg1);
    }

    


    if (PIXREGION_NIL(reg1))
    {
	if (PIXREGION_NAR(reg1))
	    return pixman_break (newReg);
        if (newReg != reg2)
	    return pixman_region_copy(newReg, reg2);
        return PIXMAN_REGION_STATUS_SUCCESS;
    }

    


    if (PIXREGION_NIL(reg2))
    {
	if (PIXREGION_NAR(reg2))
	    return pixman_break (newReg);
        if (newReg != reg1)
	    return pixman_region_copy(newReg, reg1);
        return PIXMAN_REGION_STATUS_SUCCESS;
    }

    


    if (!reg1->data && SUBSUMES(&reg1->extents, &reg2->extents))
    {
        if (newReg != reg1)
	    return pixman_region_copy(newReg, reg1);
        return PIXMAN_REGION_STATUS_SUCCESS;
    }

    


    if (!reg2->data && SUBSUMES(&reg2->extents, &reg1->extents))
    {
        if (newReg != reg2)
	    return pixman_region_copy(newReg, reg2);
        return PIXMAN_REGION_STATUS_SUCCESS;
    }

    if (!pixman_op(newReg, reg1, reg2, pixman_region_unionO, PIXMAN_REGION_STATUS_SUCCESS, PIXMAN_REGION_STATUS_SUCCESS, &overlap))
	return PIXMAN_REGION_STATUS_FAILURE;

    newReg->extents.x1 = MIN(reg1->extents.x1, reg2->extents.x1);
    newReg->extents.y1 = MIN(reg1->extents.y1, reg2->extents.y1);
    newReg->extents.x2 = MAX(reg1->extents.x2, reg2->extents.x2);
    newReg->extents.y2 = MAX(reg1->extents.y2, reg2->extents.y2);
    good(newReg);
    return PIXMAN_REGION_STATUS_SUCCESS;
}






















pixman_region_status_t
pixman_region_append(pixman_region16_t * dstrgn,
		     pixman_region16_t * rgn)
{
    int numRects, dnumRects, size;
    pixman_box16_t *new, *old;
    int prepend;

    if (PIXREGION_NAR(rgn))
	return pixman_break (dstrgn);

    if (!rgn->data && (dstrgn->data == &pixman_region_emptyData))
    {
	dstrgn->extents = rgn->extents;
	dstrgn->data = (pixman_region16_data_t *)NULL;
	return PIXMAN_REGION_STATUS_SUCCESS;
    }

    numRects = PIXREGION_NUM_RECTS(rgn);
    if (!numRects)
	return PIXMAN_REGION_STATUS_SUCCESS;
    prepend = PIXMAN_REGION_STATUS_FAILURE;
    size = numRects;
    dnumRects = PIXREGION_NUM_RECTS(dstrgn);
    if (!dnumRects && (size < 200))
	size = 200; 
    RECTALLOC(dstrgn, size);
    old = PIXREGION_RECTS(rgn);
    if (!dnumRects)
	dstrgn->extents = rgn->extents;
    else if (dstrgn->extents.x2 > dstrgn->extents.x1)
    {
	pixman_box16_t *first, *last;

	first = old;
	last = PIXREGION_BOXPTR(dstrgn) + (dnumRects - 1);
	if ((first->y1 > last->y2) ||
	    ((first->y1 == last->y1) && (first->y2 == last->y2) &&
	     (first->x1 > last->x2)))
	{
	    if (rgn->extents.x1 < dstrgn->extents.x1)
		dstrgn->extents.x1 = rgn->extents.x1;
	    if (rgn->extents.x2 > dstrgn->extents.x2)
		dstrgn->extents.x2 = rgn->extents.x2;
	    dstrgn->extents.y2 = rgn->extents.y2;
	}
	else
	{
	    first = PIXREGION_BOXPTR(dstrgn);
	    last = old + (numRects - 1);
	    if ((first->y1 > last->y2) ||
		((first->y1 == last->y1) && (first->y2 == last->y2) &&
		 (first->x1 > last->x2)))
	    {
		prepend = PIXMAN_REGION_STATUS_SUCCESS;
		if (rgn->extents.x1 < dstrgn->extents.x1)
		    dstrgn->extents.x1 = rgn->extents.x1;
		if (rgn->extents.x2 > dstrgn->extents.x2)
		    dstrgn->extents.x2 = rgn->extents.x2;
		dstrgn->extents.y1 = rgn->extents.y1;
	    }
	    else
		dstrgn->extents.x2 = dstrgn->extents.x1;
	}
    }
    if (prepend)
    {
	new = PIXREGION_BOX(dstrgn, numRects);
	if (dnumRects == 1)
	    *new = *PIXREGION_BOXPTR(dstrgn);
	else
	    memmove((char *)new,(char *)PIXREGION_BOXPTR(dstrgn),
		  dnumRects * sizeof(pixman_box16_t));
	new = PIXREGION_BOXPTR(dstrgn);
    }
    else
	new = PIXREGION_BOXPTR(dstrgn) + dnumRects;
    if (numRects == 1)
	*new = *old;
    else
	memmove((char *)new, (char *)old, numRects * sizeof(pixman_box16_t));
    dstrgn->data->numRects += numRects;
    return PIXMAN_REGION_STATUS_SUCCESS;
}

#define ExchangeRects(a, b) \
{			    \
    pixman_box16_t     t;	    \
    t = rects[a];	    \
    rects[a] = rects[b];    \
    rects[b] = t;	    \
}

static void
QuickSortRects(
    pixman_box16_t     rects[],
    int        numRects)
{
    int	y1;
    int	x1;
    int        i, j;
    pixman_box16_t *r;

    

    do
    {
	if (numRects == 2)
	{
	    if (rects[0].y1 > rects[1].y1 ||
		    (rects[0].y1 == rects[1].y1 && rects[0].x1 > rects[1].x1))
		ExchangeRects(0, 1);
	    return;
	}

	
        ExchangeRects(0, numRects >> 1);
	y1 = rects[0].y1;
	x1 = rects[0].x1;

        
        i = 0;
        j = numRects;
        do
	{
	    r = &(rects[i]);
	    do
	    {
		r++;
		i++;
            } while (i != numRects &&
		     (r->y1 < y1 || (r->y1 == y1 && r->x1 < x1)));
	    r = &(rects[j]);
	    do
	    {
		r--;
		j--;
            } while (y1 < r->y1 || (y1 == r->y1 && x1 < r->x1));
            if (i < j)
		ExchangeRects(i, j);
        } while (i < j);

        
        ExchangeRects(0, j);

	
        if (numRects-j-1 > 1)
	    QuickSortRects(&rects[j+1], numRects-j-1);
        numRects = j;
    } while (numRects > 1);
}

































pixman_region_status_t
pixman_region_validate(pixman_region16_t * badreg,
		       int *pOverlap)
{
    
    typedef struct {
	pixman_region16_t   reg;
	int	    prevBand;
	int	    curBand;
    } RegionInfo;

	     int	numRects;   
	     RegionInfo *ri;	    
    	     int	numRI;      
	     int	sizeRI;	    
	     int	i;	    
    int	j;	    
    RegionInfo *rit;       
    pixman_region16_t *  reg;        
    pixman_box16_t *	box;	    
    pixman_box16_t *	riBox;      
    pixman_region16_t *  hreg;       
    pixman_region_status_t ret = PIXMAN_REGION_STATUS_SUCCESS;

    *pOverlap = PIXMAN_REGION_STATUS_FAILURE;
    if (!badreg->data)
    {
	good(badreg);
	return PIXMAN_REGION_STATUS_SUCCESS;
    }
    numRects = badreg->data->numRects;
    if (!numRects)
    {
	if (PIXREGION_NAR(badreg))
	    return PIXMAN_REGION_STATUS_FAILURE;
	good(badreg);
	return PIXMAN_REGION_STATUS_SUCCESS;
    }
    if (badreg->extents.x1 < badreg->extents.x2)
    {
	if ((numRects) == 1)
	{
	    freeData(badreg);
	    badreg->data = (pixman_region16_data_t *) NULL;
	}
	else
	{
	    DOWNSIZE(badreg, numRects);
	}
	good(badreg);
	return PIXMAN_REGION_STATUS_SUCCESS;
    }

    
    QuickSortRects(PIXREGION_BOXPTR(badreg), numRects);

    

    
    
    ri = (RegionInfo *) malloc(4 * sizeof(RegionInfo));
    if (!ri)
	return pixman_break (badreg);
    sizeRI = 4;
    numRI = 1;
    ri[0].prevBand = 0;
    ri[0].curBand = 0;
    ri[0].reg = *badreg;
    box = PIXREGION_BOXPTR(&ri[0].reg);
    ri[0].reg.extents = *box;
    ri[0].reg.data->numRects = 1;

    





    for (i = numRects; --i > 0;)
    {
	box++;
	
	for (j = numRI, rit = ri; --j >= 0; rit++)
	{
	    reg = &rit->reg;
	    riBox = PIXREGION_END(reg);

	    if (box->y1 == riBox->y1 && box->y2 == riBox->y2)
	    {
		
		if (box->x1 <= riBox->x2)
		{
		    
		    if (box->x1 < riBox->x2) *pOverlap = PIXMAN_REGION_STATUS_SUCCESS;
		    if (box->x2 > riBox->x2) riBox->x2 = box->x2;
		}
		else
		{
		    RECTALLOC_BAIL(reg, 1, bail);
		    *PIXREGION_TOP(reg) = *box;
		    reg->data->numRects++;
		}
		goto NextRect;   
	    }
	    else if (box->y1 >= riBox->y2)
	    {
		
		if (reg->extents.x2 < riBox->x2) reg->extents.x2 = riBox->x2;
		if (reg->extents.x1 > box->x1)   reg->extents.x1 = box->x1;
		Coalesce(reg, rit->prevBand, rit->curBand);
		rit->curBand = reg->data->numRects;
		RECTALLOC_BAIL(reg, 1, bail);
		*PIXREGION_TOP(reg) = *box;
		reg->data->numRects++;
		goto NextRect;
	    }
	    
	} 

	
	if (sizeRI == numRI)
	{
	    
	    sizeRI <<= 1;
	    rit = (RegionInfo *) realloc(ri, sizeRI * sizeof(RegionInfo));
	    if (!rit)
		goto bail;
	    ri = rit;
	    rit = &ri[numRI];
	}
	numRI++;
	rit->prevBand = 0;
	rit->curBand = 0;
	rit->reg.extents = *box;
	rit->reg.data = (pixman_region16_data_t *)NULL;
	if (!pixman_rect_alloc(&rit->reg, (i+numRI) / numRI)) 
	    goto bail;
NextRect: ;
    } 

    


    for (j = numRI, rit = ri; --j >= 0; rit++)
    {
	reg = &rit->reg;
	riBox = PIXREGION_END(reg);
	reg->extents.y2 = riBox->y2;
	if (reg->extents.x2 < riBox->x2) reg->extents.x2 = riBox->x2;
	Coalesce(reg, rit->prevBand, rit->curBand);
	if (reg->data->numRects == 1) 
	{
	    freeData(reg);
	    reg->data = (pixman_region16_data_t *)NULL;
	}
    }

    
    while (numRI > 1)
    {
	int half = numRI/2;
	for (j = numRI & 1; j < (half + (numRI & 1)); j++)
	{
	    reg = &ri[j].reg;
	    hreg = &ri[j+half].reg;
	    if (!pixman_op(reg, reg, hreg, pixman_region_unionO, PIXMAN_REGION_STATUS_SUCCESS, PIXMAN_REGION_STATUS_SUCCESS, pOverlap))
		ret = PIXMAN_REGION_STATUS_FAILURE;
	    if (hreg->extents.x1 < reg->extents.x1)
		reg->extents.x1 = hreg->extents.x1;
	    if (hreg->extents.y1 < reg->extents.y1)
		reg->extents.y1 = hreg->extents.y1;
	    if (hreg->extents.x2 > reg->extents.x2)
		reg->extents.x2 = hreg->extents.x2;
	    if (hreg->extents.y2 > reg->extents.y2)
		reg->extents.y2 = hreg->extents.y2;
	    freeData(hreg);
	}
	numRI -= half;
    }
    *badreg = ri[0].reg;
    free(ri);
    good(badreg);
    return ret;
bail:
    for (i = 0; i < numRI; i++)
	freeData(&ri[i].reg);
    free (ri);
    return pixman_break (badreg);
}







































































































static pixman_region_status_t
pixman_region_subtractO (
    pixman_region16_t *	region,
    pixman_box16_t *	r1,
    pixman_box16_t *  	  	r1End,
    pixman_box16_t *	r2,
    pixman_box16_t *  	  	r2End,
    short  	y1,
             short  	y2,
    int		*pOverlap)
{
    pixman_box16_t *	pNextRect;
    int  	x1;

    x1 = r1->x1;

    assert(y1<y2);
    assert(r1 != r1End && r2 != r2End);

    pNextRect = PIXREGION_TOP(region);

    do
    {
	if (r2->x2 <= x1)
	{
	    


	    r2++;
	}
	else if (r2->x1 <= x1)
	{
	    


	    x1 = r2->x2;
	    if (x1 >= r1->x2)
	    {
		



		r1++;
		if (r1 != r1End)
		    x1 = r1->x1;
	    }
	    else
	    {
		



		r2++;
	    }
	}
	else if (r2->x1 < r1->x2)
	{
	    



	    assert(x1<r2->x1);
	    NEWRECT(region, pNextRect, x1, y1, r2->x1, y2);

	    x1 = r2->x2;
	    if (x1 >= r1->x2)
	    {
		


		r1++;
		if (r1 != r1End)
		    x1 = r1->x1;
	    }
	    else
	    {
		


		r2++;
	    }
	}
	else
	{
	    


	    if (r1->x2 > x1)
		NEWRECT(region, pNextRect, x1, y1, r1->x2, y2);
	    r1++;
	    if (r1 != r1End)
		x1 = r1->x1;
	}
    } while ((r1 != r1End) && (r2 != r2End));

    


    while (r1 != r1End)
    {
	assert(x1<r1->x2);
	NEWRECT(region, pNextRect, x1, y1, r1->x2, y2);
	r1++;
	if (r1 != r1End)
	    x1 = r1->x1;
    }
    return PIXMAN_REGION_STATUS_SUCCESS;
}















pixman_region_status_t
pixman_region_subtract(pixman_region16_t *	regD,
		       pixman_region16_t * 	regM,
		       pixman_region16_t *	regS)
{
    int overlap; 

    good(regM);
    good(regS);
    good(regD);
   
    if (PIXREGION_NIL(regM) || PIXREGION_NIL(regS) ||
	!EXTENTCHECK(&regM->extents, &regS->extents))
    {
	if (PIXREGION_NAR (regS))
	    return pixman_break (regD);
	return pixman_region_copy(regD, regM);
    }
    else if (regM == regS)
    {
	freeData(regD);
	regD->extents.x2 = regD->extents.x1;
	regD->extents.y2 = regD->extents.y1;
	regD->data = &pixman_region_emptyData;
	return PIXMAN_REGION_STATUS_SUCCESS;
    }

    


    if (!pixman_op(regD, regM, regS, pixman_region_subtractO, PIXMAN_REGION_STATUS_SUCCESS, PIXMAN_REGION_STATUS_FAILURE, &overlap))
	return PIXMAN_REGION_STATUS_FAILURE;

    






    pixman_set_extents(regD);
    good(regD);
    return PIXMAN_REGION_STATUS_SUCCESS;
}




















pixman_region_status_t
pixman_region_inverse(pixman_region16_t * 	  newReg,       
		      pixman_region16_t * 	  reg1,         
		      pixman_box16_t *     	  invRect) 	
{
    pixman_region16_t	  invReg;   	

    int	  overlap;	

    good(reg1);
    good(newReg);
   
    if (PIXREGION_NIL(reg1) || !EXTENTCHECK(invRect, &reg1->extents))
    {
	if (PIXREGION_NAR(reg1))
	    return pixman_break (newReg);
	newReg->extents = *invRect;
	freeData(newReg);
	newReg->data = (pixman_region16_data_t *)NULL;
        return PIXMAN_REGION_STATUS_SUCCESS;
    }

    


    invReg.extents = *invRect;
    invReg.data = (pixman_region16_data_t *)NULL;
    if (!pixman_op(newReg, &invReg, reg1, pixman_region_subtractO, PIXMAN_REGION_STATUS_SUCCESS, PIXMAN_REGION_STATUS_FAILURE, &overlap))
	return PIXMAN_REGION_STATUS_FAILURE;

    






    pixman_set_extents(newReg);
    good(newReg);
    return PIXMAN_REGION_STATUS_SUCCESS;
}


















int
pixman_region_contains_rectangle(pixman_region16_t *  region,
				 pixman_box16_t *     prect)
{
    int	x;
    int	y;
    pixman_box16_t *     pbox;
    pixman_box16_t *     pboxEnd;
    int			partIn, partOut;
    int			numRects;

    good(region);
    numRects = PIXREGION_NUM_RECTS(region);
    
    if (!numRects || !EXTENTCHECK(&region->extents, prect))
        return(rgnOUT);

    if (numRects == 1)
    {
	
	if (SUBSUMES(&region->extents, prect))
	    return(rgnIN);
	else
	    return(rgnPART);
    }

    partOut = PIXMAN_REGION_STATUS_FAILURE;
    partIn = PIXMAN_REGION_STATUS_FAILURE;

    
    x = prect->x1;
    y = prect->y1;

    
    for (pbox = PIXREGION_BOXPTR(region), pboxEnd = pbox + numRects;
         pbox != pboxEnd;
         pbox++)
    {

        if (pbox->y2 <= y)
           continue;    

        if (pbox->y1 > y)
        {
           partOut = PIXMAN_REGION_STATUS_SUCCESS;      
           if (partIn || (pbox->y1 >= prect->y2))
              break;
           y = pbox->y1;        
        }

        if (pbox->x2 <= x)
           continue;            

        if (pbox->x1 > x)
        {
           partOut = PIXMAN_REGION_STATUS_SUCCESS;      
           if (partIn)
              break;
        }

        if (pbox->x1 < prect->x2)
        {
            partIn = PIXMAN_REGION_STATUS_SUCCESS;      
            if (partOut)
               break;
        }

        if (pbox->x2 >= prect->x2)
        {
           y = pbox->y2;        
           if (y >= prect->y2)
              break;
           x = prect->x1;       
        }
	else
	{
	    






	    partOut = PIXMAN_REGION_STATUS_SUCCESS;
	    break;
	}
    }

    return(partIn ? ((y < prect->y2) ? rgnPART : rgnIN) : rgnOUT);
}





void
pixman_region_translate (pixman_region16_t * region, int x, int y)
{
    int x1, x2, y1, y2;
    int nbox;
    pixman_box16_t * pbox;

    good(region);
    region->extents.x1 = x1 = region->extents.x1 + x;
    region->extents.y1 = y1 = region->extents.y1 + y;
    region->extents.x2 = x2 = region->extents.x2 + x;
    region->extents.y2 = y2 = region->extents.y2 + y;
    if (((x1 - SHRT_MIN)|(y1 - SHRT_MIN)|(SHRT_MAX - x2)|(SHRT_MAX - y2)) >= 0)
    {
	if (region->data && (nbox = region->data->numRects))
	{
	    for (pbox = PIXREGION_BOXPTR(region); nbox--; pbox++)
	    {
		pbox->x1 += x;
		pbox->y1 += y;
		pbox->x2 += x;
		pbox->y2 += y;
	    }
	}
	return;
    }
    if (((x2 - SHRT_MIN)|(y2 - SHRT_MIN)|(SHRT_MAX - x1)|(SHRT_MAX - y1)) <= 0)
    {
	region->extents.x2 = region->extents.x1;
	region->extents.y2 = region->extents.y1;
	freeData(region);
	region->data = &pixman_region_emptyData;
	return;
    }
    if (x1 < SHRT_MIN)
	region->extents.x1 = SHRT_MIN;
    else if (x2 > SHRT_MAX)
	region->extents.x2 = SHRT_MAX;
    if (y1 < SHRT_MIN)
	region->extents.y1 = SHRT_MIN;
    else if (y2 > SHRT_MAX)
	region->extents.y2 = SHRT_MAX;
    if (region->data && (nbox = region->data->numRects))
    {
	pixman_box16_t * pboxout;

	for (pboxout = pbox = PIXREGION_BOXPTR(region); nbox--; pbox++)
	{
	    pboxout->x1 = x1 = pbox->x1 + x;
	    pboxout->y1 = y1 = pbox->y1 + y;
	    pboxout->x2 = x2 = pbox->x2 + x;
	    pboxout->y2 = y2 = pbox->y2 + y;
	    if (((x2 - SHRT_MIN)|(y2 - SHRT_MIN)|
		 (SHRT_MAX - x1)|(SHRT_MAX - y1)) <= 0)
	    {
		region->data->numRects--;
		continue;
	    }
	    if (x1 < SHRT_MIN)
		pboxout->x1 = SHRT_MIN;
	    else if (x2 > SHRT_MAX)
		pboxout->x2 = SHRT_MAX;
	    if (y1 < SHRT_MIN)
		pboxout->y1 = SHRT_MIN;
	    else if (y2 > SHRT_MAX)
		pboxout->y2 = SHRT_MAX;
	    pboxout++;
	}
	if (pboxout != pbox)
	{
	    if (region->data->numRects == 1)
	    {
		region->extents = *PIXREGION_BOXPTR(region);
		freeData(region);
		region->data = (pixman_region16_data_t *)NULL;
	    }
	    else
		pixman_set_extents(region);
	}
    }
}






























void
pixman_region_reset(pixman_region16_t *region, pixman_box16_t *box)
{
    good(region);
    assert(box->x1<=box->x2);
    assert(box->y1<=box->y2);
    region->extents = *box;
    freeData(region);
    region->data = (pixman_region16_data_t *)NULL;
}


int
pixman_region_contains_point(pixman_region16_t * region,
			     int x, int y,
			     pixman_box16_t * box)
{
    pixman_box16_t *pbox, *pboxEnd;
    int numRects;

    good(region);
    numRects = PIXREGION_NUM_RECTS(region);
    if (!numRects || !INBOX(&region->extents, x, y))
        return(PIXMAN_REGION_STATUS_FAILURE);
    if (numRects == 1)
    {
	*box = region->extents;
	return(PIXMAN_REGION_STATUS_SUCCESS);
    }
    for (pbox = PIXREGION_BOXPTR(region), pboxEnd = pbox + numRects;
	 pbox != pboxEnd;
	 pbox++)
    {
        if (y >= pbox->y2)
	   continue;		
	if ((y < pbox->y1) || (x < pbox->x1))
	   break;		
	if (x >= pbox->x2)
	   continue;		
	*box = *pbox;
	return(PIXMAN_REGION_STATUS_SUCCESS);
    }
    return(PIXMAN_REGION_STATUS_FAILURE);
}

int
pixman_region_not_empty(pixman_region16_t * region)
{
    good(region);
    return(!PIXREGION_NIL(region));
}










void
pixman_region_empty(pixman_region16_t * region)
{
    good(region);
    freeData(region);
    region->extents.x2 = region->extents.x1;
    region->extents.y2 = region->extents.y1;
    region->data = &pixman_region_emptyData;
}

pixman_box16_t *
pixman_region_extents(pixman_region16_t * region)
{
    good(region);
    return(&region->extents);
}

#define ExchangeSpans(a, b)				    \
{							    \
    pixman_region16_point_t tpt;					    \
    int    tw;						    \
							    \
    tpt = spans[a]; spans[a] = spans[b]; spans[b] = tpt;    \
    tw = widths[a]; widths[a] = widths[b]; widths[b] = tw;  \
}






static void QuickSortSpans(
    pixman_region16_point_t spans[],
    int	    widths[],
    int	    numSpans)
{
    int	    y;
    int	    i, j, m;
    pixman_region16_point_t *r;

    
    

    do
    {
	if (numSpans < 9)
	{
	    
	    int yprev;

	    yprev = spans[0].y;
	    i = 1;
	    do
	    { 
		y = spans[i].y;
		if (yprev > y)
		{
		    
		    pixman_region16_point_t tpt;
		    int	    tw, k;

		    for (j = 0; y >= spans[j].y; j++) {}
		    tpt = spans[i];
		    tw  = widths[i];
		    for (k = i; k != j; k--)
		    {
			spans[k] = spans[k-1];
			widths[k] = widths[k-1];
		    }
		    spans[j] = tpt;
		    widths[j] = tw;
		    y = spans[i].y;
		} 
		yprev = y;
		i++;
	    } while (i != numSpans);
	    return;
	}

	
	m = numSpans / 2;
	if (spans[m].y > spans[0].y)		ExchangeSpans(m, 0);
	if (spans[m].y > spans[numSpans-1].y)   ExchangeSpans(m, numSpans-1);
	if (spans[m].y > spans[0].y)		ExchangeSpans(m, 0);
	y = spans[0].y;

        
        i = 0;
        j = numSpans;
        do
	{
	    r = &(spans[i]);
	    do
	    {
		r++;
		i++;
            } while (i != numSpans && r->y < y);
	    r = &(spans[j]);
	    do
	    {
		r--;
		j--;
            } while (y < r->y);
            if (i < j)
		ExchangeSpans(i, j);
        } while (i < j);

        
        ExchangeSpans(0, j);

	
        if (numSpans-j-1 > 1)
	    QuickSortSpans(&spans[j+1], &widths[j+1], numSpans-j-1);
        numSpans = j;
    } while (numSpans > 1);
}

#define NextBand()						    \
{								    \
    clipy1 = pboxBandStart->y1;					    \
    clipy2 = pboxBandStart->y2;					    \
    pboxBandEnd = pboxBandStart + 1;				    \
    while (pboxBandEnd != pboxLast && pboxBandEnd->y1 == clipy1) {  \
	pboxBandEnd++;						    \
    }								    \
    for (; ppt != pptLast && ppt->y < clipy1; ppt++, pwidth++) {} \
}








#ifdef XXX_DO_WE_NEED_THIS
static int
pixman_region16_clip_spans(
    pixman_region16_t 		*prgnDst,
    pixman_region16_point_t 	*ppt,
    int	    		*pwidth,
    int			nspans,
    pixman_region16_point_t 	*pptNew,
    int			*pwidthNew,
    int			fSorted)
{
    pixman_region16_point_t 	*pptLast;
    int			*pwidthNewStart;	
    int	y, x1, x2;
    int	numRects;

    good(prgnDst);
    pptLast = ppt + nspans;
    pwidthNewStart = pwidthNew;

    if (!prgnDst->data)
    {
	
	


	   int clipx1, clipx2, clipy1, clipy2;

	clipx1 = prgnDst->extents.x1;
	clipy1 = prgnDst->extents.y1;
	clipx2 = prgnDst->extents.x2;
	clipy2 = prgnDst->extents.y2;

	for (; ppt != pptLast; ppt++, pwidth++)
	{
	    y = ppt->y;
	    x1 = ppt->x;
	    if (clipy1 <= y && y < clipy2)
	    {
		x2 = x1 + *pwidth;
		if (x1 < clipx1)    x1 = clipx1;
		if (x2 > clipx2)    x2 = clipx2;
		if (x1 < x2)
		{
		    
		    pptNew->x = x1;
		    pptNew->y = y;
		    *pwidthNew = x2 - x1;
		    pptNew++;
		    pwidthNew++;
		}
	    }
	} 

    }
    else if ((numRects = prgnDst->data->numRects))
    {
	
	pixman_box16_t *pboxBandStart, *pboxBandEnd;
	pixman_box16_t *pbox;
	pixman_box16_t *pboxLast;
	int	clipy1, clipy2;

	

	if ((! fSorted) && (nspans > 1))
	    QuickSortSpans(ppt, pwidth, nspans);

	pboxBandStart = PIXREGION_BOXPTR(prgnDst);
	pboxLast = pboxBandStart + numRects;

	NextBand();

	for (; ppt != pptLast; )
	{
	    y = ppt->y;
	    if (y < clipy2)
	    {
		
		pbox = pboxBandStart;
		x1 = ppt->x;
		x2 = x1 + *pwidth;
		do
		{ 
		    int    newx1, newx2;

		    newx1 = x1;
		    newx2 = x2;
		    if (newx1 < pbox->x1)   newx1 = pbox->x1;
		    if (newx2 > pbox->x2)   newx2 = pbox->x2;
		    if (newx1 < newx2)
		    {
			
			pptNew->x = newx1;
			pptNew->y = y;
			*pwidthNew = newx2 - newx1;
			pptNew++;
			pwidthNew++;
		    }
		    pbox++;
		} while (pbox != pboxBandEnd);
		ppt++;
		pwidth++;
	    }
	    else
	    {
		
		pboxBandStart = pboxBandEnd;
		if (pboxBandStart == pboxLast)
		    break; 
		NextBand();
	    }
	}
    }
    return (pwidthNew - pwidthNewStart);
}


static int
pixman_region16_find_max_band(pixman_region16_t * prgn)
{
    int nbox;
    pixman_box16_t * pbox;
    int nThisBand;
    int nMaxBand = 0;
    short yThisBand;

    good(prgn);
    nbox = PIXREGION_NUM_RECTS(prgn);
    pbox = PIXREGION_RECTS(prgn);

    while(nbox > 0)
    {
	yThisBand = pbox->y1;
	nThisBand = 0;
	while((nbox > 0) && (pbox->y1 == yThisBand))
	{
	    nbox--;
	    pbox++;
	    nThisBand++;
	}
	if (nThisBand > nMaxBand)
	    nMaxBand = nThisBand;
    }
    return (nMaxBand);
}
#endif 
