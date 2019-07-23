














































#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

#define PIXREGION_NIL(reg) ((reg)->data && !(reg)->data->numRects)

#define PIXREGION_NAR(reg)	((reg)->data == pixman_brokendata)
#define PIXREGION_NUM_RECTS(reg) ((reg)->data ? (reg)->data->numRects : 1)
#define PIXREGION_SIZE(reg) ((reg)->data ? (reg)->data->size : 0)
#define PIXREGION_RECTS(reg) ((reg)->data ? (box_type_t *)((reg)->data + 1) \
			               : &(reg)->extents)
#define PIXREGION_BOXPTR(reg) ((box_type_t *)((reg)->data + 1))
#define PIXREGION_BOX(reg,i) (&PIXREGION_BOXPTR(reg)[i])
#define PIXREGION_TOP(reg) PIXREGION_BOX(reg, (reg)->data->numRects)
#define PIXREGION_END(reg) PIXREGION_BOX(reg, (reg)->data->numRects - 1)


#undef assert
#ifdef DEBUG_PIXREGION
#define assert(expr) {if (!(expr)) \
		FatalError("Assertion failed file %s, line %d: expr\n", \
			__FILE__, __LINE__); }
#else
#define assert(expr)
#endif

#define good(reg) assert(PREFIX(_selfcheck) (reg))

#undef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))

static const box_type_t PREFIX(_emptyBox_) = {0, 0, 0, 0};
static const region_data_type_t PREFIX(_emptyData_) = {0, 0};
static const region_data_type_t PREFIX(_brokendata_) = {0, 0};

static box_type_t *pixman_region_emptyBox = (box_type_t *)&PREFIX(_emptyBox_);
static region_data_type_t *pixman_region_emptyData = (region_data_type_t *)&PREFIX(_emptyData_);
static region_data_type_t *pixman_brokendata = (region_data_type_t *)&PREFIX(_brokendata_);









void
PREFIX(_internal_set_static_pointers) (box_type_t *empty_box,
				       region_data_type_t *empty_data,
				       region_data_type_t *broken_data)
{
    pixman_region_emptyBox = empty_box;
    pixman_region_emptyData = empty_data;
    pixman_brokendata = broken_data;
}

static pixman_bool_t
pixman_break (region_type_t *pReg);


















































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

static size_t
PIXREGION_SZOF(size_t n)
{
    size_t size = n * sizeof(box_type_t);
    if (n > UINT32_MAX / sizeof(box_type_t))
        return 0;

    if (sizeof(region_data_type_t) > UINT32_MAX - size)
        return 0;

    return size + sizeof(region_data_type_t);
}

static void *
allocData(size_t n)
{
    size_t sz = PIXREGION_SZOF(n);
    if (!sz)
	return NULL;

    return malloc(sz);
}

#define freeData(reg) if ((reg)->data && (reg)->data->size) free((reg)->data)

#define RECTALLOC_BAIL(pReg,n,bail) \
if (!(pReg)->data || (((pReg)->data->numRects + (n)) > (pReg)->data->size)) \
    if (!pixman_rect_alloc(pReg, n)) { goto bail; }

#define RECTALLOC(pReg,n) \
if (!(pReg)->data || (((pReg)->data->numRects + (n)) > (pReg)->data->size)) \
    if (!pixman_rect_alloc(pReg, n)) { return FALSE; }

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
	    return FALSE;						\
	pNextRect = PIXREGION_TOP(pReg);					\
    }									\
    ADDRECT(pNextRect,nx1,ny1,nx2,ny2);					\
    pReg->data->numRects++;						\
    assert(pReg->data->numRects<=pReg->data->size);			\
}

#define DOWNSIZE(reg,numRects)						\
    if (((numRects) < ((reg)->data->size >> 1)) && ((reg)->data->size > 50)) \
    {									\
	region_data_type_t * NewData;				\
	size_t data_size = PIXREGION_SZOF(numRects);			\
	if (!data_size)							\
	    NewData = NULL;						\
	else								\
	    NewData = (region_data_type_t *)realloc((reg)->data, data_size); \
	if (NewData)							\
	{								\
	    NewData->size = (numRects);					\
	    (reg)->data = NewData;					\
	}								\
    }

PIXMAN_EXPORT pixman_bool_t
PREFIX(_equal) (reg1, reg2)
    region_type_t * reg1;
    region_type_t * reg2;
{
    int i;
    box_type_t *rects1;
    box_type_t *rects2;

    if (reg1->extents.x1 != reg2->extents.x1) return FALSE;
    if (reg1->extents.x2 != reg2->extents.x2) return FALSE;
    if (reg1->extents.y1 != reg2->extents.y1) return FALSE;
    if (reg1->extents.y2 != reg2->extents.y2) return FALSE;
    if (PIXREGION_NUM_RECTS(reg1) != PIXREGION_NUM_RECTS(reg2)) return FALSE;

    rects1 = PIXREGION_RECTS(reg1);
    rects2 = PIXREGION_RECTS(reg2);
    for (i = 0; i != PIXREGION_NUM_RECTS(reg1); i++) {
	if (rects1[i].x1 != rects2[i].x1) return FALSE;
	if (rects1[i].x2 != rects2[i].x2) return FALSE;
	if (rects1[i].y1 != rects2[i].y1) return FALSE;
	if (rects1[i].y2 != rects2[i].y2) return FALSE;
    }
    return TRUE;
}

int
PREFIX(_print) (rgn)
    region_type_t * rgn;
{
    int num, size;
    int i;
    box_type_t * rects;

    num = PIXREGION_NUM_RECTS(rgn);
    size = PIXREGION_SIZE(rgn);
    rects = PIXREGION_RECTS(rgn);
    fprintf(stderr, "num: %d size: %d\n", num, size);
    fprintf(stderr, "extents: %d %d %d %d\n",
	   rgn->extents.x1, rgn->extents.y1, rgn->extents.x2, rgn->extents.y2);
    for (i = 0; i < num; i++)
	fprintf(stderr, "%d %d %d %d \n",
		rects[i].x1, rects[i].y1, rects[i].x2, rects[i].y2);
    fprintf(stderr, "\n");
    return(num);
}


PIXMAN_EXPORT void
PREFIX(_init) (region_type_t *region)
{
    region->extents = *pixman_region_emptyBox;
    region->data = pixman_region_emptyData;
}

PIXMAN_EXPORT void
PREFIX(_init_rect) (region_type_t *region,
		    int x, int y, unsigned int width, unsigned int height)
{
    region->extents.x1 = x;
    region->extents.y1 = y;
    region->extents.x2 = x + width;
    region->extents.y2 = y + height;
    region->data = NULL;
}

PIXMAN_EXPORT void
PREFIX(_init_with_extents) (region_type_t *region, box_type_t *extents)
{
    region->extents = *extents;
    region->data = NULL;
}

PIXMAN_EXPORT void
PREFIX(_fini) (region_type_t *region)
{
    good (region);
    freeData (region);
}

PIXMAN_EXPORT int
PREFIX(_n_rects) (region_type_t *region)
{
    return PIXREGION_NUM_RECTS (region);
}

PIXMAN_EXPORT box_type_t *
PREFIX(_rectangles) (region_type_t *region,
				  int		    *n_rects)
{
    if (n_rects)
	*n_rects = PIXREGION_NUM_RECTS (region);

    return PIXREGION_RECTS (region);
}

static pixman_bool_t
pixman_break (region_type_t *region)
{
    freeData (region);
    region->extents = *pixman_region_emptyBox;
    region->data = pixman_brokendata;
    return FALSE;
}

static pixman_bool_t
pixman_rect_alloc (region_type_t * region, int n)
{
    region_data_type_t *data;

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
	size_t data_size;
	if (n == 1)
	{
	    n = region->data->numRects;
	    if (n > 500) 
		n = 250;
	}
	n += region->data->numRects;
	data_size = PIXREGION_SZOF(n);
	if (!data_size)
	    data = NULL;
	else
	    data = (region_data_type_t *)realloc(region->data, PIXREGION_SZOF(n));
	if (!data)
	    return pixman_break (region);
	region->data = data;
    }
    region->data->size = n;
    return TRUE;
}

PIXMAN_EXPORT pixman_bool_t
PREFIX(_copy) (region_type_t *dst, region_type_t *src)
{
    good(dst);
    good(src);
    if (dst == src)
	return TRUE;
    dst->extents = src->extents;
    if (!src->data || !src->data->size)
    {
	freeData(dst);
	dst->data = src->data;
	return TRUE;
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
	  dst->data->numRects * sizeof(box_type_t));
    return TRUE;
}























static inline int
pixman_coalesce (
    region_type_t *	region,	    	
    int	    	  	prevStart,  	
    int	    	  	curStart)   	
{
    box_type_t *	pPrevBox;   	
    box_type_t *	pCurBox;    	
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


















static inline pixman_bool_t
pixman_region_appendNonO (
    region_type_t *	region,
    box_type_t *	r,
    box_type_t *  	  	rEnd,
    int  	y1,
    int  	y2)
{
    box_type_t *	pNextRect;
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

    return TRUE;
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
              newRects * sizeof(box_type_t));				\
	newReg->data->numRects += newRects;				\
    }									\
}






























typedef pixman_bool_t (*OverlapProcPtr)(
    region_type_t	 *region,
    box_type_t *r1,
    box_type_t *r1End,
    box_type_t *r2,
    box_type_t *r2End,
    int    	 y1,
    int    	 y2,
    int		 *pOverlap);

static pixman_bool_t
pixman_op(
    region_type_t *newReg,		    
    region_type_t *       reg1,		    
    region_type_t *       reg2,		    
    OverlapProcPtr  overlapFunc,            

    int	    appendNon1,		    
					    
    int	    appendNon2,		    
					    
    int	    *pOverlap)
{
    box_type_t * r1;			    
    box_type_t * r2;			    
    box_type_t *	    r1End;		    
    box_type_t *	    r2End;		    
    int	    ybot;		    
    int	    ytop;		    
    region_data_type_t *	    oldData;		    
    int		    prevBand;		    

    int		    curBand;		    

    box_type_t * r1BandEnd;		    
    box_type_t * r2BandEnd;		    
    int	    top;		    
    int	    bot;		    
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

    oldData = (region_data_type_t *)NULL;
    if (((newReg == reg1) && (newSize > 1)) ||
	((newReg == reg2) && (numRects > 1)))
    {
	oldData = newReg->data;
	newReg->data = pixman_region_emptyData;
    }
    
    if (numRects > newSize)
	newSize = numRects;
    newSize <<= 1;
    if (!newReg->data)
	newReg->data = pixman_region_emptyData;
    else if (newReg->data->size)
	newReg->data->numRects = 0;
    if (newSize > newReg->data->size) {
	if (!pixman_rect_alloc(newReg, newSize)) {
	    if (oldData)
		free (oldData);
	    return FALSE;
	}
    }

    













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
	newReg->data = pixman_region_emptyData;
    }
    else if (numRects == 1)
    {
	newReg->extents = *PIXREGION_BOXPTR(newReg);
	freeData(newReg);
	newReg->data = (region_data_type_t *)NULL;
    }
    else
    {
	DOWNSIZE(newReg, numRects);
    }

    return TRUE;
}
















static void
pixman_set_extents (region_type_t *region)
{
    box_type_t *box, *boxEnd;

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


















static pixman_bool_t
pixman_region_intersectO (region_type_t *region,
			  box_type_t    *r1,
			  box_type_t    *r1End,
			  box_type_t    *r2,
			  box_type_t    *r2End,
			  int    	     y1,
			  int    	     y2,
			  int		    *pOverlap)
{
    int  	x1;
    int  	x2;
    box_type_t *	pNextRect;

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

    return TRUE;
}

PIXMAN_EXPORT pixman_bool_t
PREFIX(_intersect) (region_type_t * 	newReg,
			 region_type_t * 	reg1,
			 region_type_t *	reg2)
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
	    newReg->data = pixman_brokendata;
	    return FALSE;
	}
	else
	    newReg->data = pixman_region_emptyData;
    }
    else if (!reg1->data && !reg2->data)
    {
	
	newReg->extents.x1 = MAX(reg1->extents.x1, reg2->extents.x1);
	newReg->extents.y1 = MAX(reg1->extents.y1, reg2->extents.y1);
	newReg->extents.x2 = MIN(reg1->extents.x2, reg2->extents.x2);
	newReg->extents.y2 = MIN(reg1->extents.y2, reg2->extents.y2);
	freeData(newReg);
	newReg->data = (region_data_type_t *)NULL;
    }
    else if (!reg2->data && SUBSUMES(&reg2->extents, &reg1->extents))
    {
	return PREFIX(_copy) (newReg, reg1);
    }
    else if (!reg1->data && SUBSUMES(&reg1->extents, &reg2->extents))
    {
	return PREFIX(_copy) (newReg, reg2);
    }
    else if (reg1 == reg2)
    {
	return PREFIX(_copy) (newReg, reg1);
    }
    else
    {
	
	int overlap; 
	if (!pixman_op(newReg, reg1, reg2, pixman_region_intersectO, FALSE, FALSE,
			&overlap))
	    return FALSE;
	pixman_set_extents(newReg);
    }

    good(newReg);
    return(TRUE);
}

#define MERGERECT(r)						\
{								\
    if (r->x1 <= x2) {						\
	/* Merge with current rectangle */			\
	if (r->x1 < x2) *pOverlap = TRUE;				\
	if (x2 < r->x2) x2 = r->x2;				\
    } else {							\
	/* Add current rectangle, start new one */		\
	NEWRECT(region, pNextRect, x1, y1, x2, y2);		\
	x1 = r->x1;						\
	x2 = r->x2;						\
    }								\
    r++;							\
}




















static pixman_bool_t
pixman_region_unionO (
    region_type_t	 *region,
    box_type_t *r1,
    box_type_t *r1End,
    box_type_t *r2,
    box_type_t *r2End,
    int	  y1,
    int	  y2,
    int		  *pOverlap)
{
    box_type_t *     pNextRect;
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

    return TRUE;
}




PIXMAN_EXPORT pixman_bool_t
PREFIX(_union_rect) (region_type_t *dest,
			  region_type_t *source,
			  int x, int y,
			  unsigned int width, unsigned int height)
{
    region_type_t region;

    if (!width || !height)
	return PREFIX(_copy) (dest, source);
    region.data = NULL;
    region.extents.x1 = x;
    region.extents.y1 = y;
    region.extents.x2 = x + width;
    region.extents.y2 = y + height;

    return PREFIX(_union) (dest, source, &region);
}

PIXMAN_EXPORT pixman_bool_t
PREFIX(_union) (region_type_t *newReg,
		     region_type_t *reg1,
		     region_type_t *reg2)
{
    int overlap; 

    


    good(reg1);
    good(reg2);
    good(newReg);
    

    


    if (reg1 == reg2)
    {
	return PREFIX(_copy) (newReg, reg1);
    }

    


    if (PIXREGION_NIL(reg1))
    {
	if (PIXREGION_NAR(reg1))
	    return pixman_break (newReg);
        if (newReg != reg2)
	    return PREFIX(_copy) (newReg, reg2);
        return TRUE;
    }

    


    if (PIXREGION_NIL(reg2))
    {
	if (PIXREGION_NAR(reg2))
	    return pixman_break (newReg);
        if (newReg != reg1)
	    return PREFIX(_copy) (newReg, reg1);
        return TRUE;
    }

    


    if (!reg1->data && SUBSUMES(&reg1->extents, &reg2->extents))
    {
        if (newReg != reg1)
	    return PREFIX(_copy) (newReg, reg1);
        return TRUE;
    }

    


    if (!reg2->data && SUBSUMES(&reg2->extents, &reg1->extents))
    {
        if (newReg != reg2)
	    return PREFIX(_copy) (newReg, reg2);
        return TRUE;
    }

    if (!pixman_op(newReg, reg1, reg2, pixman_region_unionO, TRUE, TRUE, &overlap))
	return FALSE;

    newReg->extents.x1 = MIN(reg1->extents.x1, reg2->extents.x1);
    newReg->extents.y1 = MIN(reg1->extents.y1, reg2->extents.y1);
    newReg->extents.x2 = MAX(reg1->extents.x2, reg2->extents.x2);
    newReg->extents.y2 = MAX(reg1->extents.y2, reg2->extents.y2);
    good(newReg);
    return TRUE;
}





#define ExchangeRects(a, b) \
{			    \
    box_type_t     t;	    \
    t = rects[a];	    \
    rects[a] = rects[b];    \
    rects[b] = t;	    \
}

static void
QuickSortRects(
    box_type_t     rects[],
    int        numRects)
{
    int	y1;
    int	x1;
    int        i, j;
    box_type_t *r;

    

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


































static pixman_bool_t
validate (region_type_t * badreg,
	  int *pOverlap)
{
    
    typedef struct {
	region_type_t   reg;
	int	    prevBand;
	int	    curBand;
    } RegionInfo;

    RegionInfo stack_regions[64];

	     int	numRects;   
	     RegionInfo *ri;	    
    	     int	numRI;      
	     int	sizeRI;	    
	     int	i;	    
    int	j;	    
    RegionInfo *rit;       
    region_type_t *  reg;        
    box_type_t *	box;	    
    box_type_t *	riBox;      
    region_type_t *  hreg;       
    pixman_bool_t ret = TRUE;

    *pOverlap = FALSE;
    if (!badreg->data)
    {
	good(badreg);
	return TRUE;
    }
    numRects = badreg->data->numRects;
    if (!numRects)
    {
	if (PIXREGION_NAR(badreg))
	    return FALSE;
	good(badreg);
	return TRUE;
    }
    if (badreg->extents.x1 < badreg->extents.x2)
    {
	if ((numRects) == 1)
	{
	    freeData(badreg);
	    badreg->data = (region_data_type_t *) NULL;
	}
	else
	{
	    DOWNSIZE(badreg, numRects);
	}
	good(badreg);
	return TRUE;
    }

    
    QuickSortRects(PIXREGION_BOXPTR(badreg), numRects);

    

    
    
    ri = stack_regions;
    sizeRI = sizeof (stack_regions) / sizeof (stack_regions[0]);
    numRI = 1;
    ri[0].prevBand = 0;
    ri[0].curBand = 0;
    ri[0].reg = *badreg;
    box = PIXREGION_BOXPTR(&ri[0].reg);
    ri[0].reg.extents = *box;
    ri[0].reg.data->numRects = 1;
    badreg->extents = *pixman_region_emptyBox;
    badreg->data = pixman_region_emptyData;

    





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
		    
		    if (box->x1 < riBox->x2) *pOverlap = TRUE;
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
	    size_t data_size;
	    
	    
	    sizeRI <<= 1;

            data_size = sizeRI * sizeof(RegionInfo);
            if (data_size / sizeRI != sizeof(RegionInfo))
                goto bail;
	    if (ri == stack_regions) {
		rit = malloc (data_size);
		if (!rit)
		    goto bail;
		memcpy (rit, ri, numRI * sizeof (RegionInfo));
	    } else {
		rit = (RegionInfo *) realloc(ri, data_size);
		if (!rit)
		    goto bail;
	    }
	    ri = rit;
	    rit = &ri[numRI];
	}
	numRI++;
	rit->prevBand = 0;
	rit->curBand = 0;
	rit->reg.extents = *box;
	rit->reg.data = (region_data_type_t *)NULL;
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
	    reg->data = (region_data_type_t *)NULL;
	}
    }

    
    while (numRI > 1)
    {
	int half = numRI/2;
	for (j = numRI & 1; j < (half + (numRI & 1)); j++)
	{
	    reg = &ri[j].reg;
	    hreg = &ri[j+half].reg;
	    if (!pixman_op(reg, reg, hreg, pixman_region_unionO, TRUE, TRUE, pOverlap))
		ret = FALSE;
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
	if (!ret)
	    goto bail;
    }
    *badreg = ri[0].reg;
    if (ri != stack_regions)
	free(ri);
    good(badreg);
    return ret;
bail:
    for (i = 0; i < numRI; i++)
	freeData(&ri[i].reg);
    if (ri != stack_regions)
	free (ri);

    return pixman_break (badreg);
}




















static pixman_bool_t
pixman_region_subtractO (
    region_type_t *	region,
    box_type_t *	r1,
    box_type_t *  	  	r1End,
    box_type_t *	r2,
    box_type_t *  	  	r2End,
    int  	y1,
    int  	y2,
    int		*pOverlap)
{
    box_type_t *	pNextRect;
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
    return TRUE;
}















PIXMAN_EXPORT pixman_bool_t
PREFIX(_subtract) (region_type_t *	regD,
		       region_type_t * 	regM,
		       region_type_t *	regS)
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
	return PREFIX(_copy) (regD, regM);
    }
    else if (regM == regS)
    {
	freeData(regD);
	regD->extents.x2 = regD->extents.x1;
	regD->extents.y2 = regD->extents.y1;
	regD->data = pixman_region_emptyData;
	return TRUE;
    }

    


    if (!pixman_op(regD, regM, regS, pixman_region_subtractO, TRUE, FALSE, &overlap))
	return FALSE;

    






    pixman_set_extents(regD);
    good(regD);
    return TRUE;
}




















pixman_bool_t
PIXMAN_EXPORT PREFIX(_inverse) (region_type_t * 	  newReg,       
		      region_type_t * 	  reg1,         
		      box_type_t *     	  invRect) 	
{
    region_type_t	  invReg;   	

    int	  overlap;	

    good(reg1);
    good(newReg);
   
    if (PIXREGION_NIL(reg1) || !EXTENTCHECK(invRect, &reg1->extents))
    {
	if (PIXREGION_NAR(reg1))
	    return pixman_break (newReg);
	newReg->extents = *invRect;
	freeData(newReg);
	newReg->data = (region_data_type_t *)NULL;
        return TRUE;
    }

    


    invReg.extents = *invRect;
    invReg.data = (region_data_type_t *)NULL;
    if (!pixman_op(newReg, &invReg, reg1, pixman_region_subtractO, TRUE, FALSE, &overlap))
	return FALSE;

    






    pixman_set_extents(newReg);
    good(newReg);
    return TRUE;
}


















pixman_region_overlap_t
PIXMAN_EXPORT PREFIX(_contains_rectangle) (region_type_t *  region,
				 box_type_t *     prect)
{
    int	x;
    int	y;
    box_type_t *     pbox;
    box_type_t *     pboxEnd;
    int			partIn, partOut;
    int			numRects;

    good(region);
    numRects = PIXREGION_NUM_RECTS(region);
    
    if (!numRects || !EXTENTCHECK(&region->extents, prect))
        return(PIXMAN_REGION_OUT);

    if (numRects == 1)
    {
	
	if (SUBSUMES(&region->extents, prect))
	    return(PIXMAN_REGION_IN);
	else
	    return(PIXMAN_REGION_PART);
    }

    partOut = FALSE;
    partIn = FALSE;

    
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
           partOut = TRUE;      
           if (partIn || (pbox->y1 >= prect->y2))
              break;
           y = pbox->y1;        
        }

        if (pbox->x2 <= x)
           continue;            

        if (pbox->x1 > x)
        {
           partOut = TRUE;      
           if (partIn)
              break;
        }

        if (pbox->x1 < prect->x2)
        {
            partIn = TRUE;      
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
	    






	    partOut = TRUE;
	    break;
	}
    }

    if (partIn)
    {
	if (y < prect->y2)
	    return PIXMAN_REGION_PART;
	else
	    return PIXMAN_REGION_IN;
    }
    else
    {
	return PIXMAN_REGION_OUT;
    }
}





PIXMAN_EXPORT void
PREFIX(_translate) (region_type_t * region, int x, int y)
{
    int x1, x2, y1, y2;
    int nbox;
    box_type_t * pbox;

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
	region->data = pixman_region_emptyData;
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
	box_type_t * pboxout;

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
		region->data = (region_data_type_t *)NULL;
	    }
	    else
		pixman_set_extents(region);
	}
    }
}

PIXMAN_EXPORT void
PREFIX(_reset) (region_type_t *region, box_type_t *box)
{
    good(region);
    assert(box->x1<=box->x2);
    assert(box->y1<=box->y2);
    region->extents = *box;
    freeData(region);
    region->data = (region_data_type_t *)NULL;
}


PIXMAN_EXPORT int
PREFIX(_contains_point) (region_type_t * region,
			     int x, int y,
			     box_type_t * box)
{
    box_type_t *pbox, *pboxEnd;
    int numRects;

    good(region);
    numRects = PIXREGION_NUM_RECTS(region);
    if (!numRects || !INBOX(&region->extents, x, y))
        return(FALSE);
    if (numRects == 1)
    {
        if (box)
	    *box = region->extents;

	return(TRUE);
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

        if (box)
	    *box = *pbox;

	return(TRUE);
    }
    return(FALSE);
}

PIXMAN_EXPORT int
PREFIX(_not_empty) (region_type_t * region)
{
    good(region);
    return(!PIXREGION_NIL(region));
}

PIXMAN_EXPORT box_type_t *
PREFIX(_extents) (region_type_t * region)
{
    good(region);
    return(&region->extents);
}








PIXMAN_EXPORT pixman_bool_t
PREFIX(_selfcheck) (reg)
    region_type_t * reg;
{
    int i, numRects;

    if ((reg->extents.x1 > reg->extents.x2) ||
	(reg->extents.y1 > reg->extents.y2))
	return FALSE;
    numRects = PIXREGION_NUM_RECTS(reg);
    if (!numRects)
	return ((reg->extents.x1 == reg->extents.x2) &&
		(reg->extents.y1 == reg->extents.y2) &&
		(reg->data->size || (reg->data == pixman_region_emptyData)));
    else if (numRects == 1)
	return (!reg->data);
    else
    {
	box_type_t * pboxP, * pboxN;
	box_type_t box;

	pboxP = PIXREGION_RECTS(reg);
	box = *pboxP;
	box.y2 = pboxP[numRects-1].y2;
	pboxN = pboxP + 1;
	for (i = numRects; --i > 0; pboxP++, pboxN++)
	{
	    if ((pboxN->x1 >= pboxN->x2) ||
		(pboxN->y1 >= pboxN->y2))
		return FALSE;
	    if (pboxN->x1 < box.x1)
	        box.x1 = pboxN->x1;
	    if (pboxN->x2 > box.x2)
		box.x2 = pboxN->x2;
	    if ((pboxN->y1 < pboxP->y1) ||
		((pboxN->y1 == pboxP->y1) &&
		 ((pboxN->x1 < pboxP->x2) || (pboxN->y2 != pboxP->y2))))
		return FALSE;
	}
	return ((box.x1 == reg->extents.x1) &&
		(box.x2 == reg->extents.x2) &&
		(box.y1 == reg->extents.y1) &&
		(box.y2 == reg->extents.y2));
    }
}

PIXMAN_EXPORT pixman_bool_t
PREFIX(_init_rects) (region_type_t *region,
		     box_type_t *boxes, int count)
{
    int overlap;

    

    if (count == 1) {
       PREFIX(_init_rect) (region,
                               boxes[0].x1,
                               boxes[0].y1,
                               boxes[0].x2 - boxes[0].x1,
                               boxes[0].y2 - boxes[0].y1);
       return TRUE;
    }

    PREFIX(_init) (region);

    




    if (count == 0)
        return TRUE;

    if (!pixman_rect_alloc(region, count))
	return FALSE;

    
    memcpy (PIXREGION_RECTS(region), boxes, sizeof(box_type_t) * count);
    region->data->numRects = count;

    
    region->extents.x1 = region->extents.x2 = 0;
    return validate (region, &overlap);
}
