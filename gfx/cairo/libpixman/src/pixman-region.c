
































































#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include "pixman-private.h"

#define PIXREGION_NIL(reg) ((reg)->data && !(reg)->data->numRects)

#define PIXREGION_NAR(reg)      ((reg)->data == pixman_broken_data)
#define PIXREGION_NUMRECTS(reg) ((reg)->data ? (reg)->data->numRects : 1)
#define PIXREGION_SIZE(reg) ((reg)->data ? (reg)->data->size : 0)
#define PIXREGION_RECTS(reg) \
    ((reg)->data ? (box_type_t *)((reg)->data + 1) \
     : &(reg)->extents)
#define PIXREGION_BOXPTR(reg) ((box_type_t *)((reg)->data + 1))
#define PIXREGION_BOX(reg, i) (&PIXREGION_BOXPTR (reg)[i])
#define PIXREGION_TOP(reg) PIXREGION_BOX (reg, (reg)->data->numRects)
#define PIXREGION_END(reg) PIXREGION_BOX (reg, (reg)->data->numRects - 1)

#define GOOD_RECT(rect) ((rect)->x1 < (rect)->x2 && (rect)->y1 < (rect)->y2)
#define BAD_RECT(rect) ((rect)->x1 > (rect)->x2 || (rect)->y1 > (rect)->y2)

#ifdef DEBUG

#define GOOD(reg)							\
    do									\
    {									\
	if (!PREFIX (_selfcheck (reg)))					\
	    _pixman_log_error (FUNC, "Malformed region " # reg);	\
    } while (0)

#else

#define GOOD(reg)

#endif

static const box_type_t PREFIX (_empty_box_) = { 0, 0, 0, 0 };
static const region_data_type_t PREFIX (_empty_data_) = { 0, 0 };
#if defined (__llvm__) && !defined (__clang__)
static const volatile region_data_type_t PREFIX (_broken_data_) = { 0, 0 };
#else
static const region_data_type_t PREFIX (_broken_data_) = { 0, 0 };
#endif

static box_type_t *pixman_region_empty_box =
    (box_type_t *)&PREFIX (_empty_box_);
static region_data_type_t *pixman_region_empty_data =
    (region_data_type_t *)&PREFIX (_empty_data_);
static region_data_type_t *pixman_broken_data =
    (region_data_type_t *)&PREFIX (_broken_data_);

static pixman_bool_t
pixman_break (region_type_t *region);



















































#define EXTENTCHECK(r1, r2)	   \
    (!( ((r1)->x2 <= (r2)->x1)  || \
        ((r1)->x1 >= (r2)->x2)  || \
        ((r1)->y2 <= (r2)->y1)  || \
        ((r1)->y1 >= (r2)->y2) ) )


#define INBOX(r, x, y)	\
    ( ((r)->x2 >  x) && \
      ((r)->x1 <= x) && \
      ((r)->y2 >  y) && \
      ((r)->y1 <= y) )


#define SUBSUMES(r1, r2)	\
    ( ((r1)->x1 <= (r2)->x1) && \
      ((r1)->x2 >= (r2)->x2) && \
      ((r1)->y1 <= (r2)->y1) && \
      ((r1)->y2 >= (r2)->y2) )

static size_t
PIXREGION_SZOF (size_t n)
{
    size_t size = n * sizeof(box_type_t);
    
    if (n > UINT32_MAX / sizeof(box_type_t))
	return 0;

    if (sizeof(region_data_type_t) > UINT32_MAX - size)
	return 0;

    return size + sizeof(region_data_type_t);
}

static region_data_type_t *
alloc_data (size_t n)
{
    size_t sz = PIXREGION_SZOF (n);

    if (!sz)
	return NULL;

    return malloc (sz);
}

#define FREE_DATA(reg) if ((reg)->data && (reg)->data->size) free ((reg)->data)

#define RECTALLOC_BAIL(region, n, bail)					\
    do									\
    {									\
	if (!(region)->data ||						\
	    (((region)->data->numRects + (n)) > (region)->data->size))	\
	{								\
	    if (!pixman_rect_alloc (region, n))				\
		goto bail;						\
	}								\
    } while (0)

#define RECTALLOC(region, n)						\
    do									\
    {									\
	if (!(region)->data ||						\
	    (((region)->data->numRects + (n)) > (region)->data->size))	\
	{								\
	    if (!pixman_rect_alloc (region, n)) {			\
		return FALSE;						\
	    }								\
	}								\
    } while (0)

#define ADDRECT(next_rect, nx1, ny1, nx2, ny2)      \
    do						    \
    {						    \
	next_rect->x1 = nx1;                        \
	next_rect->y1 = ny1;                        \
	next_rect->x2 = nx2;                        \
	next_rect->y2 = ny2;                        \
	next_rect++;                                \
    }						    \
    while (0)

#define NEWRECT(region, next_rect, nx1, ny1, nx2, ny2)			\
    do									\
    {									\
	if (!(region)->data ||						\
	    ((region)->data->numRects == (region)->data->size))		\
	{								\
	    if (!pixman_rect_alloc (region, 1))				\
		return FALSE;						\
	    next_rect = PIXREGION_TOP (region);				\
	}								\
	ADDRECT (next_rect, nx1, ny1, nx2, ny2);			\
	region->data->numRects++;					\
	critical_if_fail (region->data->numRects <= region->data->size);		\
    } while (0)

#define DOWNSIZE(reg, numRects)						\
    do									\
    {									\
	if (((numRects) < ((reg)->data->size >> 1)) &&			\
	    ((reg)->data->size > 50))					\
	{								\
	    region_data_type_t * new_data;				\
	    size_t data_size = PIXREGION_SZOF (numRects);		\
									\
	    if (!data_size)						\
	    {								\
		new_data = NULL;					\
	    }								\
	    else							\
	    {								\
		new_data = (region_data_type_t *)			\
		    realloc ((reg)->data, data_size);			\
	    }								\
									\
	    if (new_data)						\
	    {								\
		new_data->size = (numRects);				\
		(reg)->data = new_data;					\
	    }								\
	}								\
    } while (0)

PIXMAN_EXPORT pixman_bool_t
PREFIX (_equal) (region_type_t *reg1, region_type_t *reg2)
{
    int i;
    box_type_t *rects1;
    box_type_t *rects2;

    if (reg1->extents.x1 != reg2->extents.x1)
	return FALSE;
    
    if (reg1->extents.x2 != reg2->extents.x2)
	return FALSE;
    
    if (reg1->extents.y1 != reg2->extents.y1)
	return FALSE;
    
    if (reg1->extents.y2 != reg2->extents.y2)
	return FALSE;
    
    if (PIXREGION_NUMRECTS (reg1) != PIXREGION_NUMRECTS (reg2))
	return FALSE;

    rects1 = PIXREGION_RECTS (reg1);
    rects2 = PIXREGION_RECTS (reg2);
    
    for (i = 0; i != PIXREGION_NUMRECTS (reg1); i++)
    {
	if (rects1[i].x1 != rects2[i].x1)
	    return FALSE;
	
	if (rects1[i].x2 != rects2[i].x2)
	    return FALSE;
	
	if (rects1[i].y1 != rects2[i].y1)
	    return FALSE;
	
	if (rects1[i].y2 != rects2[i].y2)
	    return FALSE;
    }

    return TRUE;
}

int
PREFIX (_print) (region_type_t *rgn)
{
    int num, size;
    int i;
    box_type_t * rects;

    num = PIXREGION_NUMRECTS (rgn);
    size = PIXREGION_SIZE (rgn);
    rects = PIXREGION_RECTS (rgn);

    fprintf (stderr, "num: %d size: %d\n", num, size);
    fprintf (stderr, "extents: %d %d %d %d\n",
             rgn->extents.x1,
	     rgn->extents.y1,
	     rgn->extents.x2,
	     rgn->extents.y2);
    
    for (i = 0; i < num; i++)
    {
	fprintf (stderr, "%d %d %d %d \n",
	         rects[i].x1, rects[i].y1, rects[i].x2, rects[i].y2);
    }
    
    fprintf (stderr, "\n");

    return(num);
}


PIXMAN_EXPORT void
PREFIX (_init) (region_type_t *region)
{
    region->extents = *pixman_region_empty_box;
    region->data = pixman_region_empty_data;
}

PIXMAN_EXPORT void
PREFIX (_init_rect) (region_type_t *	region,
                     int		x,
		     int		y,
		     unsigned int	width,
		     unsigned int	height)
{
    region->extents.x1 = x;
    region->extents.y1 = y;
    region->extents.x2 = x + width;
    region->extents.y2 = y + height;

    if (!GOOD_RECT (&region->extents))
    {
        if (BAD_RECT (&region->extents))
            _pixman_log_error (FUNC, "Invalid rectangle passed");
        PREFIX (_init) (region);
        return;
    }

    region->data = NULL;
}

PIXMAN_EXPORT void
PREFIX (_init_with_extents) (region_type_t *region, box_type_t *extents)
{
    if (!GOOD_RECT (extents))
    {
        if (BAD_RECT (extents))
            _pixman_log_error (FUNC, "Invalid rectangle passed");
        PREFIX (_init) (region);
        return;
    }
    region->extents = *extents;

    region->data = NULL;
}

PIXMAN_EXPORT void
PREFIX (_fini) (region_type_t *region)
{
    GOOD (region);
    FREE_DATA (region);
}

PIXMAN_EXPORT int
PREFIX (_n_rects) (region_type_t *region)
{
    return PIXREGION_NUMRECTS (region);
}

PIXMAN_EXPORT box_type_t *
PREFIX (_rectangles) (region_type_t *region,
                      int               *n_rects)
{
    if (n_rects)
	*n_rects = PIXREGION_NUMRECTS (region);

    return PIXREGION_RECTS (region);
}

static pixman_bool_t
pixman_break (region_type_t *region)
{
    FREE_DATA (region);

    region->extents = *pixman_region_empty_box;
    region->data = pixman_broken_data;

    return FALSE;
}

static pixman_bool_t
pixman_rect_alloc (region_type_t * region,
                   int             n)
{
    region_data_type_t *data;

    if (!region->data)
    {
	n++;
	region->data = alloc_data (n);

	if (!region->data)
	    return pixman_break (region);

	region->data->numRects = 1;
	*PIXREGION_BOXPTR (region) = region->extents;
    }
    else if (!region->data->size)
    {
	region->data = alloc_data (n);

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
	data_size = PIXREGION_SZOF (n);

	if (!data_size)
	{
	    data = NULL;
	}
	else
	{
	    data = (region_data_type_t *)
		realloc (region->data, PIXREGION_SZOF (n));
	}
	
	if (!data)
	    return pixman_break (region);
	
	region->data = data;
    }
    
    region->data->size = n;

    return TRUE;
}

PIXMAN_EXPORT pixman_bool_t
PREFIX (_copy) (region_type_t *dst, region_type_t *src)
{
    GOOD (dst);
    GOOD (src);

    if (dst == src)
	return TRUE;
    
    dst->extents = src->extents;

    if (!src->data || !src->data->size)
    {
	FREE_DATA (dst);
	dst->data = src->data;
	return TRUE;
    }
    
    if (!dst->data || (dst->data->size < src->data->numRects))
    {
	FREE_DATA (dst);

	dst->data = alloc_data (src->data->numRects);

	if (!dst->data)
	    return pixman_break (dst);

	dst->data->size = src->data->numRects;
    }

    dst->data->numRects = src->data->numRects;

    memmove ((char *)PIXREGION_BOXPTR (dst), (char *)PIXREGION_BOXPTR (src),
             dst->data->numRects * sizeof(box_type_t));

    return TRUE;
}























static inline int
pixman_coalesce (region_type_t * region,      
		 int             prev_start,  
		 int             cur_start)   
{
    box_type_t *prev_box;       
    box_type_t *cur_box;        
    int numRects;               
    int y2;                     

    


    numRects = cur_start - prev_start;
    critical_if_fail (numRects == region->data->numRects - cur_start);

    if (!numRects) return cur_start;

    



    prev_box = PIXREGION_BOX (region, prev_start);
    cur_box = PIXREGION_BOX (region, cur_start);
    if (prev_box->y2 != cur_box->y1) return cur_start;

    





    y2 = cur_box->y2;

    do
    {
	if ((prev_box->x1 != cur_box->x1) || (prev_box->x2 != cur_box->x2))
	    return (cur_start);
	
	prev_box++;
	cur_box++;
	numRects--;
    }
    while (numRects);

    



    numRects = cur_start - prev_start;
    region->data->numRects -= numRects;

    do
    {
	prev_box--;
	prev_box->y2 = y2;
	numRects--;
    }
    while (numRects);

    return prev_start;
}



#define COALESCE(new_reg, prev_band, cur_band)                          \
    do									\
    {									\
	if (cur_band - prev_band == new_reg->data->numRects - cur_band)	\
	    prev_band = pixman_coalesce (new_reg, prev_band, cur_band);	\
	else								\
	    prev_band = cur_band;					\
    } while (0)

















static inline pixman_bool_t
pixman_region_append_non_o (region_type_t * region,
			    box_type_t *    r,
			    box_type_t *    r_end,
			    int             y1,
			    int             y2)
{
    box_type_t *next_rect;
    int new_rects;

    new_rects = r_end - r;

    critical_if_fail (y1 < y2);
    critical_if_fail (new_rects != 0);

    
    RECTALLOC (region, new_rects);
    next_rect = PIXREGION_TOP (region);
    region->data->numRects += new_rects;

    do
    {
	critical_if_fail (r->x1 < r->x2);
	ADDRECT (next_rect, r->x1, y1, r->x2, y2);
	r++;
    }
    while (r != r_end);

    return TRUE;
}

#define FIND_BAND(r, r_band_end, r_end, ry1)			     \
    do								     \
    {								     \
	ry1 = r->y1;						     \
	r_band_end = r + 1;					     \
	while ((r_band_end != r_end) && (r_band_end->y1 == ry1)) {   \
	    r_band_end++;					     \
	}							     \
    } while (0)

#define APPEND_REGIONS(new_reg, r, r_end)				\
    do									\
    {									\
	int new_rects;							\
	if ((new_rects = r_end - r)) {					\
	    RECTALLOC_BAIL (new_reg, new_rects, bail);			\
	    memmove ((char *)PIXREGION_TOP (new_reg), (char *)r,	\
		     new_rects * sizeof(box_type_t));			\
	    new_reg->data->numRects += new_rects;			\
	}								\
    } while (0)






























typedef pixman_bool_t (*overlap_proc_ptr) (region_type_t *region,
					   box_type_t *   r1,
					   box_type_t *   r1_end,
					   box_type_t *   r2,
					   box_type_t *   r2_end,
					   int            y1,
					   int            y2);

static pixman_bool_t
pixman_op (region_type_t *  new_reg,               
	   region_type_t *  reg1,                  
	   region_type_t *  reg2,                  
	   overlap_proc_ptr overlap_func,          

	   int              append_non1,           


	   int              append_non2            


    )
{
    box_type_t *r1;                 
    box_type_t *r2;                 
    box_type_t *r1_end;             
    box_type_t *r2_end;             
    int ybot;                       
    int ytop;                       
    region_data_type_t *old_data;   
    int prev_band;                  

    int cur_band;                   

    box_type_t * r1_band_end;       
    box_type_t * r2_band_end;       
    int top;                        
    int bot;                        
    int r1y1;                       
    int r2y1;
    int new_size;
    int numRects;

    


    if (PIXREGION_NAR (reg1) || PIXREGION_NAR (reg2))
	return pixman_break (new_reg);

    







    r1 = PIXREGION_RECTS (reg1);
    new_size = PIXREGION_NUMRECTS (reg1);
    r1_end = r1 + new_size;

    numRects = PIXREGION_NUMRECTS (reg2);
    r2 = PIXREGION_RECTS (reg2);
    r2_end = r2 + numRects;
    
    critical_if_fail (r1 != r1_end);
    critical_if_fail (r2 != r2_end);

    old_data = (region_data_type_t *)NULL;

    if (((new_reg == reg1) && (new_size > 1)) ||
        ((new_reg == reg2) && (numRects > 1)))
    {
        old_data = new_reg->data;
        new_reg->data = pixman_region_empty_data;
    }

    
    if (numRects > new_size)
	new_size = numRects;

    new_size <<= 1;

    if (!new_reg->data)
	new_reg->data = pixman_region_empty_data;
    else if (new_reg->data->size)
	new_reg->data->numRects = 0;

    if (new_size > new_reg->data->size)
    {
        if (!pixman_rect_alloc (new_reg, new_size))
        {
            free (old_data);
            return FALSE;
	}
    }

    













    ybot = MIN (r1->y1, r2->y1);

    








    prev_band = 0;

    do
    {
        






        critical_if_fail (r1 != r1_end);
        critical_if_fail (r2 != r2_end);

        FIND_BAND (r1, r1_band_end, r1_end, r1y1);
        FIND_BAND (r2, r2_band_end, r2_end, r2y1);

        







        if (r1y1 < r2y1)
        {
            if (append_non1)
            {
                top = MAX (r1y1, ybot);
                bot = MIN (r1->y2, r2y1);
                if (top != bot)
                {
                    cur_band = new_reg->data->numRects;
                    if (!pixman_region_append_non_o (new_reg, r1, r1_band_end, top, bot))
			goto bail;
                    COALESCE (new_reg, prev_band, cur_band);
		}
	    }
            ytop = r2y1;
	}
        else if (r2y1 < r1y1)
        {
            if (append_non2)
            {
                top = MAX (r2y1, ybot);
                bot = MIN (r2->y2, r1y1);
		
                if (top != bot)
                {
                    cur_band = new_reg->data->numRects;

                    if (!pixman_region_append_non_o (new_reg, r2, r2_band_end, top, bot))
			goto bail;

                    COALESCE (new_reg, prev_band, cur_band);
		}
	    }
            ytop = r1y1;
	}
        else
        {
            ytop = r1y1;
	}

        



        ybot = MIN (r1->y2, r2->y2);
        if (ybot > ytop)
        {
            cur_band = new_reg->data->numRects;

            if (!(*overlap_func)(new_reg,
                                 r1, r1_band_end,
                                 r2, r2_band_end,
                                 ytop, ybot))
	    {
		goto bail;
	    }
	    
            COALESCE (new_reg, prev_band, cur_band);
	}

        



        if (r1->y2 == ybot)
	    r1 = r1_band_end;

        if (r2->y2 == ybot)
	    r2 = r2_band_end;

    }
    while (r1 != r1_end && r2 != r2_end);

    







    if ((r1 != r1_end) && append_non1)
    {
        
        FIND_BAND (r1, r1_band_end, r1_end, r1y1);
	
        cur_band = new_reg->data->numRects;
	
        if (!pixman_region_append_non_o (new_reg,
                                         r1, r1_band_end,
                                         MAX (r1y1, ybot), r1->y2))
	{
	    goto bail;
	}
	
        COALESCE (new_reg, prev_band, cur_band);

        
        APPEND_REGIONS (new_reg, r1_band_end, r1_end);
    }
    else if ((r2 != r2_end) && append_non2)
    {
        
        FIND_BAND (r2, r2_band_end, r2_end, r2y1);

	cur_band = new_reg->data->numRects;

        if (!pixman_region_append_non_o (new_reg,
                                         r2, r2_band_end,
                                         MAX (r2y1, ybot), r2->y2))
	{
	    goto bail;
	}

        COALESCE (new_reg, prev_band, cur_band);

        
        APPEND_REGIONS (new_reg, r2_band_end, r2_end);
    }

    free (old_data);

    if (!(numRects = new_reg->data->numRects))
    {
        FREE_DATA (new_reg);
        new_reg->data = pixman_region_empty_data;
    }
    else if (numRects == 1)
    {
        new_reg->extents = *PIXREGION_BOXPTR (new_reg);
        FREE_DATA (new_reg);
        new_reg->data = (region_data_type_t *)NULL;
    }
    else
    {
        DOWNSIZE (new_reg, numRects);
    }

    return TRUE;

bail:
    free (old_data);

    return pixman_break (new_reg);
}
















static void
pixman_set_extents (region_type_t *region)
{
    box_type_t *box, *box_end;

    if (!region->data)
	return;

    if (!region->data->size)
    {
        region->extents.x2 = region->extents.x1;
        region->extents.y2 = region->extents.y1;
        return;
    }

    box = PIXREGION_BOXPTR (region);
    box_end = PIXREGION_END (region);

    






    region->extents.x1 = box->x1;
    region->extents.y1 = box->y1;
    region->extents.x2 = box_end->x2;
    region->extents.y2 = box_end->y2;

    critical_if_fail (region->extents.y1 < region->extents.y2);

    while (box <= box_end)
    {
        if (box->x1 < region->extents.x1)
	    region->extents.x1 = box->x1;
        if (box->x2 > region->extents.x2)
	    region->extents.x2 = box->x2;
        box++;
    }

    critical_if_fail (region->extents.x1 < region->extents.x2);
}


















static pixman_bool_t
pixman_region_intersect_o (region_type_t *region,
                           box_type_t *   r1,
                           box_type_t *   r1_end,
                           box_type_t *   r2,
                           box_type_t *   r2_end,
                           int            y1,
                           int            y2)
{
    int x1;
    int x2;
    box_type_t *        next_rect;

    next_rect = PIXREGION_TOP (region);

    critical_if_fail (y1 < y2);
    critical_if_fail (r1 != r1_end && r2 != r2_end);

    do
    {
        x1 = MAX (r1->x1, r2->x1);
        x2 = MIN (r1->x2, r2->x2);

        



        if (x1 < x2)
	    NEWRECT (region, next_rect, x1, y1, x2, y2);

        




        if (r1->x2 == x2)
        {
            r1++;
	}
        if (r2->x2 == x2)
        {
            r2++;
	}
    }
    while ((r1 != r1_end) && (r2 != r2_end));

    return TRUE;
}

PIXMAN_EXPORT pixman_bool_t
PREFIX (_intersect) (region_type_t *     new_reg,
                     region_type_t *        reg1,
                     region_type_t *        reg2)
{
    GOOD (reg1);
    GOOD (reg2);
    GOOD (new_reg);

    
    if (PIXREGION_NIL (reg1) || PIXREGION_NIL (reg2) ||
        !EXTENTCHECK (&reg1->extents, &reg2->extents))
    {
        
        FREE_DATA (new_reg);
        new_reg->extents.x2 = new_reg->extents.x1;
        new_reg->extents.y2 = new_reg->extents.y1;
        if (PIXREGION_NAR (reg1) || PIXREGION_NAR (reg2))
        {
            new_reg->data = pixman_broken_data;
            return FALSE;
	}
        else
	{
	    new_reg->data = pixman_region_empty_data;
	}
    }
    else if (!reg1->data && !reg2->data)
    {
        
        new_reg->extents.x1 = MAX (reg1->extents.x1, reg2->extents.x1);
        new_reg->extents.y1 = MAX (reg1->extents.y1, reg2->extents.y1);
        new_reg->extents.x2 = MIN (reg1->extents.x2, reg2->extents.x2);
        new_reg->extents.y2 = MIN (reg1->extents.y2, reg2->extents.y2);

        FREE_DATA (new_reg);

	new_reg->data = (region_data_type_t *)NULL;
    }
    else if (!reg2->data && SUBSUMES (&reg2->extents, &reg1->extents))
    {
        return PREFIX (_copy) (new_reg, reg1);
    }
    else if (!reg1->data && SUBSUMES (&reg1->extents, &reg2->extents))
    {
        return PREFIX (_copy) (new_reg, reg2);
    }
    else if (reg1 == reg2)
    {
        return PREFIX (_copy) (new_reg, reg1);
    }
    else
    {
        

        if (!pixman_op (new_reg, reg1, reg2, pixman_region_intersect_o, FALSE, FALSE))
	    return FALSE;
	
        pixman_set_extents (new_reg);
    }

    GOOD (new_reg);
    return(TRUE);
}

#define MERGERECT(r)							\
    do									\
    {									\
        if (r->x1 <= x2)						\
	{								\
            /* Merge with current rectangle */				\
            if (x2 < r->x2)						\
		x2 = r->x2;						\
	}								\
	else								\
	{								\
            /* Add current rectangle, start new one */			\
            NEWRECT (region, next_rect, x1, y1, x2, y2);		\
            x1 = r->x1;							\
            x2 = r->x2;							\
	}								\
        r++;								\
    } while (0)




















static pixman_bool_t
pixman_region_union_o (region_type_t *region,
		       box_type_t *   r1,
		       box_type_t *   r1_end,
		       box_type_t *   r2,
		       box_type_t *   r2_end,
		       int            y1,
		       int            y2)
{
    box_type_t *next_rect;
    int x1;            
    int x2;

    critical_if_fail (y1 < y2);
    critical_if_fail (r1 != r1_end && r2 != r2_end);

    next_rect = PIXREGION_TOP (region);

    
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
    while (r1 != r1_end && r2 != r2_end)
    {
        if (r1->x1 < r2->x1)
	    MERGERECT (r1);
	else
	    MERGERECT (r2);
    }

    
    if (r1 != r1_end)
    {
        do
        {
            MERGERECT (r1);
	}
        while (r1 != r1_end);
    }
    else if (r2 != r2_end)
    {
        do
        {
            MERGERECT (r2);
	}
        while (r2 != r2_end);
    }

    
    NEWRECT (region, next_rect, x1, y1, x2, y2);

    return TRUE;
}

PIXMAN_EXPORT pixman_bool_t
PREFIX(_intersect_rect) (region_type_t *dest,
			 region_type_t *source,
			 int x, int y,
			 unsigned int width,
			 unsigned int height)
{
    region_type_t region;

    region.data = NULL;
    region.extents.x1 = x;
    region.extents.y1 = y;
    region.extents.x2 = x + width;
    region.extents.y2 = y + height;

    return PREFIX(_intersect) (dest, source, &region);
}




PIXMAN_EXPORT pixman_bool_t
PREFIX (_union_rect) (region_type_t *dest,
                      region_type_t *source,
                      int            x,
		      int            y,
                      unsigned int   width,
		      unsigned int   height)
{
    region_type_t region;

    region.extents.x1 = x;
    region.extents.y1 = y;
    region.extents.x2 = x + width;
    region.extents.y2 = y + height;

    if (!GOOD_RECT (&region.extents))
    {
        if (BAD_RECT (&region.extents))
            _pixman_log_error (FUNC, "Invalid rectangle passed");
	return PREFIX (_copy) (dest, source);
    }

    region.data = NULL;

    return PREFIX (_union) (dest, source, &region);
}

PIXMAN_EXPORT pixman_bool_t
PREFIX (_union) (region_type_t *new_reg,
                 region_type_t *reg1,
                 region_type_t *reg2)
{
    


    GOOD (reg1);
    GOOD (reg2);
    GOOD (new_reg);

    

    


    if (reg1 == reg2)
        return PREFIX (_copy) (new_reg, reg1);

    


    if (PIXREGION_NIL (reg1))
    {
        if (PIXREGION_NAR (reg1))
	    return pixman_break (new_reg);

        if (new_reg != reg2)
	    return PREFIX (_copy) (new_reg, reg2);

	return TRUE;
    }

    


    if (PIXREGION_NIL (reg2))
    {
        if (PIXREGION_NAR (reg2))
	    return pixman_break (new_reg);

	if (new_reg != reg1)
	    return PREFIX (_copy) (new_reg, reg1);

	return TRUE;
    }

    


    if (!reg1->data && SUBSUMES (&reg1->extents, &reg2->extents))
    {
        if (new_reg != reg1)
	    return PREFIX (_copy) (new_reg, reg1);

	return TRUE;
    }

    


    if (!reg2->data && SUBSUMES (&reg2->extents, &reg1->extents))
    {
        if (new_reg != reg2)
	    return PREFIX (_copy) (new_reg, reg2);

	return TRUE;
    }

    if (!pixman_op (new_reg, reg1, reg2, pixman_region_union_o, TRUE, TRUE))
	return FALSE;

    new_reg->extents.x1 = MIN (reg1->extents.x1, reg2->extents.x1);
    new_reg->extents.y1 = MIN (reg1->extents.y1, reg2->extents.y1);
    new_reg->extents.x2 = MAX (reg1->extents.x2, reg2->extents.x2);
    new_reg->extents.y2 = MAX (reg1->extents.y2, reg2->extents.y2);
    
    GOOD (new_reg);

    return TRUE;
}





#define EXCHANGE_RECTS(a, b)	\
    {                           \
        box_type_t t;		\
        t = rects[a];           \
        rects[a] = rects[b];    \
        rects[b] = t;           \
    }

static void
quick_sort_rects (
    box_type_t rects[],
    int        numRects)
{
    int y1;
    int x1;
    int i, j;
    box_type_t *r;

    

    do
    {
        if (numRects == 2)
        {
            if (rects[0].y1 > rects[1].y1 ||
                (rects[0].y1 == rects[1].y1 && rects[0].x1 > rects[1].x1))
	    {
		EXCHANGE_RECTS (0, 1);
	    }

            return;
	}

        
        EXCHANGE_RECTS (0, numRects >> 1);
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
	    }
	    while (i != numRects && (r->y1 < y1 || (r->y1 == y1 && r->x1 < x1)));

	    r = &(rects[j]);
            do
            {
                r--;
                j--;
	    }
            while (y1 < r->y1 || (y1 == r->y1 && x1 < r->x1));
	    
            if (i < j)
		EXCHANGE_RECTS (i, j);
	}
        while (i < j);

        
        EXCHANGE_RECTS (0, j);

        
        if (numRects - j - 1 > 1)
	    quick_sort_rects (&rects[j + 1], numRects - j - 1);

        numRects = j;
    }
    while (numRects > 1);
}


































static pixman_bool_t
validate (region_type_t * badreg)
{
    
    typedef struct
    {
        region_type_t reg;
        int prev_band;
        int cur_band;
    } region_info_t;

    region_info_t stack_regions[64];

    int numRects;                   
    region_info_t *ri;              
    int num_ri;                     
    int size_ri;                    
    int i;                          
    int j;                          
    region_info_t *rit;             
    region_type_t *reg;             
    box_type_t *box;                
    box_type_t *ri_box;             
    region_type_t *hreg;            
    pixman_bool_t ret = TRUE;

    if (!badreg->data)
    {
        GOOD (badreg);
        return TRUE;
    }
    
    numRects = badreg->data->numRects;
    if (!numRects)
    {
        if (PIXREGION_NAR (badreg))
	    return FALSE;
        GOOD (badreg);
        return TRUE;
    }
    
    if (badreg->extents.x1 < badreg->extents.x2)
    {
        if ((numRects) == 1)
        {
            FREE_DATA (badreg);
            badreg->data = (region_data_type_t *) NULL;
	}
        else
        {
            DOWNSIZE (badreg, numRects);
	}

        GOOD (badreg);

	return TRUE;
    }

    
    quick_sort_rects (PIXREGION_BOXPTR (badreg), numRects);

    

    
    
    ri = stack_regions;
    size_ri = sizeof (stack_regions) / sizeof (stack_regions[0]);
    num_ri = 1;
    ri[0].prev_band = 0;
    ri[0].cur_band = 0;
    ri[0].reg = *badreg;
    box = PIXREGION_BOXPTR (&ri[0].reg);
    ri[0].reg.extents = *box;
    ri[0].reg.data->numRects = 1;
    badreg->extents = *pixman_region_empty_box;
    badreg->data = pixman_region_empty_data;

    






    for (i = numRects; --i > 0;)
    {
        box++;
        
        for (j = num_ri, rit = ri; --j >= 0; rit++)
        {
            reg = &rit->reg;
            ri_box = PIXREGION_END (reg);

            if (box->y1 == ri_box->y1 && box->y2 == ri_box->y2)
            {
                
                if (box->x1 <= ri_box->x2)
                {
                    
                    if (box->x2 > ri_box->x2)
			ri_box->x2 = box->x2;
		}
                else
                {
                    RECTALLOC_BAIL (reg, 1, bail);
                    *PIXREGION_TOP (reg) = *box;
                    reg->data->numRects++;
		}
		
                goto next_rect;   
	    }
            else if (box->y1 >= ri_box->y2)
            {
                
                if (reg->extents.x2 < ri_box->x2)
		    reg->extents.x2 = ri_box->x2;
		
                if (reg->extents.x1 > box->x1)
		    reg->extents.x1 = box->x1;
		
                COALESCE (reg, rit->prev_band, rit->cur_band);
                rit->cur_band = reg->data->numRects;
                RECTALLOC_BAIL (reg, 1, bail);
                *PIXREGION_TOP (reg) = *box;
                reg->data->numRects++;

                goto next_rect;
	    }
            
	} 

        
        if (size_ri == num_ri)
        {
            size_t data_size;

            
            size_ri <<= 1;

            data_size = size_ri * sizeof(region_info_t);
            if (data_size / size_ri != sizeof(region_info_t))
		goto bail;

            if (ri == stack_regions)
            {
                rit = malloc (data_size);
                if (!rit)
		    goto bail;
                memcpy (rit, ri, num_ri * sizeof (region_info_t));
	    }
            else
            {
                rit = (region_info_t *) realloc (ri, data_size);
                if (!rit)
		    goto bail;
	    }
            ri = rit;
            rit = &ri[num_ri];
	}
        num_ri++;
        rit->prev_band = 0;
        rit->cur_band = 0;
        rit->reg.extents = *box;
        rit->reg.data = (region_data_type_t *)NULL;

	
        if (!pixman_rect_alloc (&rit->reg, (i + num_ri) / num_ri))
	    goto bail;
	
    next_rect: ;
    } 

    


    for (j = num_ri, rit = ri; --j >= 0; rit++)
    {
        reg = &rit->reg;
        ri_box = PIXREGION_END (reg);
        reg->extents.y2 = ri_box->y2;

        if (reg->extents.x2 < ri_box->x2)
	    reg->extents.x2 = ri_box->x2;
	
        COALESCE (reg, rit->prev_band, rit->cur_band);

	if (reg->data->numRects == 1) 
        {
            FREE_DATA (reg);
            reg->data = (region_data_type_t *)NULL;
	}
    }

    
    while (num_ri > 1)
    {
        int half = num_ri / 2;
        for (j = num_ri & 1; j < (half + (num_ri & 1)); j++)
        {
            reg = &ri[j].reg;
            hreg = &ri[j + half].reg;

            if (!pixman_op (reg, reg, hreg, pixman_region_union_o, TRUE, TRUE))
		ret = FALSE;

            if (hreg->extents.x1 < reg->extents.x1)
		reg->extents.x1 = hreg->extents.x1;

            if (hreg->extents.y1 < reg->extents.y1)
		reg->extents.y1 = hreg->extents.y1;

            if (hreg->extents.x2 > reg->extents.x2)
		reg->extents.x2 = hreg->extents.x2;

            if (hreg->extents.y2 > reg->extents.y2)
		reg->extents.y2 = hreg->extents.y2;

            FREE_DATA (hreg);
	}

        num_ri -= half;

	if (!ret)
	    goto bail;
    }

    *badreg = ri[0].reg;

    if (ri != stack_regions)
	free (ri);

    GOOD (badreg);
    return ret;

bail:
    for (i = 0; i < num_ri; i++)
	FREE_DATA (&ri[i].reg);

    if (ri != stack_regions)
	free (ri);

    return pixman_break (badreg);
}




















static pixman_bool_t
pixman_region_subtract_o (region_type_t * region,
			  box_type_t *    r1,
			  box_type_t *    r1_end,
			  box_type_t *    r2,
			  box_type_t *    r2_end,
			  int             y1,
			  int             y2)
{
    box_type_t *        next_rect;
    int x1;

    x1 = r1->x1;

    critical_if_fail (y1 < y2);
    critical_if_fail (r1 != r1_end && r2 != r2_end);

    next_rect = PIXREGION_TOP (region);

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
                if (r1 != r1_end)
		    x1 = r1->x1;
	    }
            else
            {
                



                r2++;
	    }
	}
        else if (r2->x1 < r1->x2)
        {
            



            critical_if_fail (x1 < r2->x1);
            NEWRECT (region, next_rect, x1, y1, r2->x1, y2);

            x1 = r2->x2;
            if (x1 >= r1->x2)
            {
                


                r1++;
                if (r1 != r1_end)
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
		NEWRECT (region, next_rect, x1, y1, r1->x2, y2);

            r1++;

	    if (r1 != r1_end)
		x1 = r1->x1;
	}
    }
    while ((r1 != r1_end) && (r2 != r2_end));

    


    while (r1 != r1_end)
    {
        critical_if_fail (x1 < r1->x2);

        NEWRECT (region, next_rect, x1, y1, r1->x2, y2);

        r1++;
        if (r1 != r1_end)
	    x1 = r1->x1;
    }
    return TRUE;
}















PIXMAN_EXPORT pixman_bool_t
PREFIX (_subtract) (region_type_t *reg_d,
                    region_type_t *reg_m,
                    region_type_t *reg_s)
{
    GOOD (reg_m);
    GOOD (reg_s);
    GOOD (reg_d);
    
    
    if (PIXREGION_NIL (reg_m) || PIXREGION_NIL (reg_s) ||
        !EXTENTCHECK (&reg_m->extents, &reg_s->extents))
    {
        if (PIXREGION_NAR (reg_s))
	    return pixman_break (reg_d);
	
        return PREFIX (_copy) (reg_d, reg_m);
    }
    else if (reg_m == reg_s)
    {
        FREE_DATA (reg_d);
        reg_d->extents.x2 = reg_d->extents.x1;
        reg_d->extents.y2 = reg_d->extents.y1;
        reg_d->data = pixman_region_empty_data;

        return TRUE;
    }

    


    if (!pixman_op (reg_d, reg_m, reg_s, pixman_region_subtract_o, TRUE, FALSE))
	return FALSE;

    






    pixman_set_extents (reg_d);
    GOOD (reg_d);
    return TRUE;
}




















PIXMAN_EXPORT pixman_bool_t
PREFIX (_inverse) (region_type_t *new_reg,  
		   region_type_t *reg1,     
		   box_type_t *   inv_rect) 
{
    region_type_t inv_reg; 

    GOOD (reg1);
    GOOD (new_reg);
    
    
    if (PIXREGION_NIL (reg1) || !EXTENTCHECK (inv_rect, &reg1->extents))
    {
        if (PIXREGION_NAR (reg1))
	    return pixman_break (new_reg);
	
        new_reg->extents = *inv_rect;
        FREE_DATA (new_reg);
        new_reg->data = (region_data_type_t *)NULL;
	
        return TRUE;
    }

    



    inv_reg.extents = *inv_rect;
    inv_reg.data = (region_data_type_t *)NULL;
    if (!pixman_op (new_reg, &inv_reg, reg1, pixman_region_subtract_o, TRUE, FALSE))
	return FALSE;

    






    pixman_set_extents (new_reg);
    GOOD (new_reg);
    return TRUE;
}




static box_type_t *
find_box_for_y (box_type_t *begin, box_type_t *end, int y)
{
    box_type_t *mid;

    if (end == begin)
	return end;

    if (end - begin == 1)
    {
	if (begin->y2 > y)
	    return begin;
	else
	    return end;
    }

    mid = begin + (end - begin) / 2;
    if (mid->y2 > y)
    {
	



	return find_box_for_y (begin, mid, y);
    }
    else
    {
	return find_box_for_y (mid, end, y);
    }
}

















PIXMAN_EXPORT pixman_region_overlap_t
PREFIX (_contains_rectangle) (region_type_t *  region,
			      box_type_t *     prect)
{
    box_type_t *     pbox;
    box_type_t *     pbox_end;
    int part_in, part_out;
    int numRects;
    int x, y;

    GOOD (region);

    numRects = PIXREGION_NUMRECTS (region);

    
    if (!numRects || !EXTENTCHECK (&region->extents, prect))
	return(PIXMAN_REGION_OUT);

    if (numRects == 1)
    {
        
        if (SUBSUMES (&region->extents, prect))
	    return(PIXMAN_REGION_IN);
        else
	    return(PIXMAN_REGION_PART);
    }

    part_out = FALSE;
    part_in = FALSE;

    
    x = prect->x1;
    y = prect->y1;

    
    for (pbox = PIXREGION_BOXPTR (region), pbox_end = pbox + numRects;
	 pbox != pbox_end;
	 pbox++)
    {
	
	if (pbox->y2 <= y)
	{
	    if ((pbox = find_box_for_y (pbox, pbox_end, y)) == pbox_end)
		break;
	}

        if (pbox->y1 > y)
        {
            part_out = TRUE;     
            if (part_in || (pbox->y1 >= prect->y2))
		break;
            y = pbox->y1;       
	}

        if (pbox->x2 <= x)
	    continue;           

        if (pbox->x1 > x)
        {
            part_out = TRUE;     
            if (part_in)
		break;
	}

        if (pbox->x1 < prect->x2)
        {
            part_in = TRUE;      
            if (part_out)
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
            






            part_out = TRUE;
            break;
	}
    }

    if (part_in)
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
PREFIX (_translate) (region_type_t *region, int x, int y)
{
    overflow_int_t x1, x2, y1, y2;
    int nbox;
    box_type_t * pbox;

    GOOD (region);
    region->extents.x1 = x1 = region->extents.x1 + x;
    region->extents.y1 = y1 = region->extents.y1 + y;
    region->extents.x2 = x2 = region->extents.x2 + x;
    region->extents.y2 = y2 = region->extents.y2 + y;
    
    if (((x1 - PIXMAN_REGION_MIN) | (y1 - PIXMAN_REGION_MIN) | (PIXMAN_REGION_MAX - x2) | (PIXMAN_REGION_MAX - y2)) >= 0)
    {
        if (region->data && (nbox = region->data->numRects))
        {
            for (pbox = PIXREGION_BOXPTR (region); nbox--; pbox++)
            {
                pbox->x1 += x;
                pbox->y1 += y;
                pbox->x2 += x;
                pbox->y2 += y;
	    }
	}
        return;
    }

    if (((x2 - PIXMAN_REGION_MIN) | (y2 - PIXMAN_REGION_MIN) | (PIXMAN_REGION_MAX - x1) | (PIXMAN_REGION_MAX - y1)) <= 0)
    {
        region->extents.x2 = region->extents.x1;
        region->extents.y2 = region->extents.y1;
        FREE_DATA (region);
        region->data = pixman_region_empty_data;
        return;
    }

    if (x1 < PIXMAN_REGION_MIN)
	region->extents.x1 = PIXMAN_REGION_MIN;
    else if (x2 > PIXMAN_REGION_MAX)
	region->extents.x2 = PIXMAN_REGION_MAX;

    if (y1 < PIXMAN_REGION_MIN)
	region->extents.y1 = PIXMAN_REGION_MIN;
    else if (y2 > PIXMAN_REGION_MAX)
	region->extents.y2 = PIXMAN_REGION_MAX;

    if (region->data && (nbox = region->data->numRects))
    {
        box_type_t * pbox_out;

        for (pbox_out = pbox = PIXREGION_BOXPTR (region); nbox--; pbox++)
        {
            pbox_out->x1 = x1 = pbox->x1 + x;
            pbox_out->y1 = y1 = pbox->y1 + y;
            pbox_out->x2 = x2 = pbox->x2 + x;
            pbox_out->y2 = y2 = pbox->y2 + y;

            if (((x2 - PIXMAN_REGION_MIN) | (y2 - PIXMAN_REGION_MIN) |
                 (PIXMAN_REGION_MAX - x1) | (PIXMAN_REGION_MAX - y1)) <= 0)
            {
                region->data->numRects--;
                continue;
	    }

            if (x1 < PIXMAN_REGION_MIN)
		pbox_out->x1 = PIXMAN_REGION_MIN;
            else if (x2 > PIXMAN_REGION_MAX)
		pbox_out->x2 = PIXMAN_REGION_MAX;

            if (y1 < PIXMAN_REGION_MIN)
		pbox_out->y1 = PIXMAN_REGION_MIN;
            else if (y2 > PIXMAN_REGION_MAX)
		pbox_out->y2 = PIXMAN_REGION_MAX;

            pbox_out++;
	}

        if (pbox_out != pbox)
        {
            if (region->data->numRects == 1)
            {
                region->extents = *PIXREGION_BOXPTR (region);
                FREE_DATA (region);
                region->data = (region_data_type_t *)NULL;
	    }
            else
	    {
		pixman_set_extents (region);
	    }
	}
    }

    GOOD (region);
}

PIXMAN_EXPORT void
PREFIX (_reset) (region_type_t *region, box_type_t *box)
{
    GOOD (region);

    critical_if_fail (GOOD_RECT (box));

    region->extents = *box;

    FREE_DATA (region);

    region->data = NULL;
}

PIXMAN_EXPORT void
PREFIX (_clear) (region_type_t *region)
{
    GOOD (region);
    FREE_DATA (region);

    region->extents = *pixman_region_empty_box;
    region->data = pixman_region_empty_data;
}


PIXMAN_EXPORT int
PREFIX (_contains_point) (region_type_t * region,
                          int x, int y,
                          box_type_t * box)
{
    box_type_t *pbox, *pbox_end;
    int numRects;

    GOOD (region);
    numRects = PIXREGION_NUMRECTS (region);

    if (!numRects || !INBOX (&region->extents, x, y))
	return(FALSE);

    if (numRects == 1)
    {
        if (box)
	    *box = region->extents;

        return(TRUE);
    }

    pbox = PIXREGION_BOXPTR (region);
    pbox_end = pbox + numRects;

    pbox = find_box_for_y (pbox, pbox_end, y);

    for (;pbox != pbox_end; pbox++)
    {
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
PREFIX (_not_empty) (region_type_t * region)
{
    GOOD (region);

    return(!PIXREGION_NIL (region));
}

PIXMAN_EXPORT box_type_t *
PREFIX (_extents) (region_type_t * region)
{
    GOOD (region);

    return(&region->extents);
}








PIXMAN_EXPORT pixman_bool_t
PREFIX (_selfcheck) (region_type_t *reg)
{
    int i, numRects;

    if ((reg->extents.x1 > reg->extents.x2) ||
        (reg->extents.y1 > reg->extents.y2))
    {
	return FALSE;
    }

    numRects = PIXREGION_NUMRECTS (reg);
    if (!numRects)
    {
	return ((reg->extents.x1 == reg->extents.x2) &&
	        (reg->extents.y1 == reg->extents.y2) &&
	        (reg->data->size || (reg->data == pixman_region_empty_data)));
    }
    else if (numRects == 1)
    {
	return (!reg->data);
    }
    else
    {
        box_type_t * pbox_p, * pbox_n;
        box_type_t box;

        pbox_p = PIXREGION_RECTS (reg);
        box = *pbox_p;
        box.y2 = pbox_p[numRects - 1].y2;
        pbox_n = pbox_p + 1;

        for (i = numRects; --i > 0; pbox_p++, pbox_n++)
        {
            if ((pbox_n->x1 >= pbox_n->x2) ||
                (pbox_n->y1 >= pbox_n->y2))
	    {
		return FALSE;
	    }

            if (pbox_n->x1 < box.x1)
		box.x1 = pbox_n->x1;
	    
            if (pbox_n->x2 > box.x2)
		box.x2 = pbox_n->x2;
	    
            if ((pbox_n->y1 < pbox_p->y1) ||
                ((pbox_n->y1 == pbox_p->y1) &&
                 ((pbox_n->x1 < pbox_p->x2) || (pbox_n->y2 != pbox_p->y2))))
	    {
		return FALSE;
	    }
	}

        return ((box.x1 == reg->extents.x1) &&
                (box.x2 == reg->extents.x2) &&
                (box.y1 == reg->extents.y1) &&
                (box.y2 == reg->extents.y2));
    }
}

PIXMAN_EXPORT pixman_bool_t
PREFIX (_init_rects) (region_type_t *region,
                      const box_type_t *boxes, int count)
{
    box_type_t *rects;
    int displacement;
    int i;

    

    if (count == 1)
    {
        PREFIX (_init_rect) (region,
                             boxes[0].x1,
                             boxes[0].y1,
                             boxes[0].x2 - boxes[0].x1,
                             boxes[0].y2 - boxes[0].y1);
        return TRUE;
    }

    PREFIX (_init) (region);

    




    if (count == 0)
	return TRUE;

    if (!pixman_rect_alloc (region, count))
	return FALSE;

    rects = PIXREGION_RECTS (region);

    
    memcpy (rects, boxes, sizeof(box_type_t) * count);
    region->data->numRects = count;

    
    displacement = 0;

    for (i = 0; i < count; ++i)
    {
        box_type_t *box = &rects[i];

        if (box->x1 >= box->x2 || box->y1 >= box->y2)
	    displacement++;
        else if (displacement)
	    rects[i - displacement] = rects[i];
    }

    region->data->numRects -= displacement;

    


    if (region->data->numRects == 0)
    {
        FREE_DATA (region);
        PREFIX (_init) (region);

        return TRUE;
    }

    if (region->data->numRects == 1)
    {
        region->extents = rects[0];

        FREE_DATA (region);
        region->data = NULL;

        GOOD (region);

        return TRUE;
    }

    
    region->extents.x1 = region->extents.x2 = 0;

    return validate (region);
}

#define READ(_ptr) (*(_ptr))

static inline box_type_t *
bitmap_addrect (region_type_t *reg,
                box_type_t *r,
                box_type_t **first_rect,
                int rx1, int ry1,
                int rx2, int ry2)
{
    if ((rx1 < rx2) && (ry1 < ry2) &&
	(!(reg->data->numRects &&
	   ((r-1)->y1 == ry1) && ((r-1)->y2 == ry2) &&
	   ((r-1)->x1 <= rx1) && ((r-1)->x2 >= rx2))))
    {
	if (reg->data->numRects == reg->data->size)
	{
	    if (!pixman_rect_alloc (reg, 1))
		return NULL;
	    *first_rect = PIXREGION_BOXPTR(reg);
	    r = *first_rect + reg->data->numRects;
	}
	r->x1 = rx1;
	r->y1 = ry1;
	r->x2 = rx2;
	r->y2 = ry2;
	reg->data->numRects++;
	if (r->x1 < reg->extents.x1)
	    reg->extents.x1 = r->x1;
	if (r->x2 > reg->extents.x2)
	    reg->extents.x2 = r->x2;
	r++;
    }
    return r;
}








PIXMAN_EXPORT void
PREFIX (_init_from_image) (region_type_t *region,
                           pixman_image_t *image)
{
    uint32_t mask0 = 0xffffffff & ~SCREEN_SHIFT_RIGHT(0xffffffff, 1);
    box_type_t *first_rect, *rects, *prect_line_start;
    box_type_t *old_rect, *new_rect;
    uint32_t *pw, w, *pw_line, *pw_line_end;
    int	irect_prev_start, irect_line_start;
    int	h, base, rx1 = 0, crects;
    int	ib;
    pixman_bool_t in_box, same;
    int width, height, stride;

    PREFIX(_init) (region);

    critical_if_fail (region->data);

    return_if_fail (image->type == BITS);
    return_if_fail (image->bits.format == PIXMAN_a1);

    pw_line = pixman_image_get_data (image);
    width = pixman_image_get_width (image);
    height = pixman_image_get_height (image);
    stride = pixman_image_get_stride (image) / 4;

    first_rect = PIXREGION_BOXPTR(region);
    rects = first_rect;

    region->extents.x1 = width - 1;
    region->extents.x2 = 0;
    irect_prev_start = -1;
    for (h = 0; h < height; h++)
    {
        pw = pw_line;
        pw_line += stride;
        irect_line_start = rects - first_rect;

        

        if (READ(pw) & mask0)
        {
            in_box = TRUE;
            rx1 = 0;
        }
        else
        {
            in_box = FALSE;
        }

        
        pw_line_end = pw + (width >> 5);
        for (base = 0; pw < pw_line_end; base += 32)
        {
            w = READ(pw++);
            if (in_box)
            {
                if (!~w)
                    continue;
            }
            else
            {
                if (!w)
                    continue;
            }
            for (ib = 0; ib < 32; ib++)
            {
                

                if (w & mask0)
                {
                    if (!in_box)
                    {
                        rx1 = base + ib;
                        
                        in_box = TRUE;
                    }
                }
                else
                {
                    if (in_box)
                    {
                        
                        rects = bitmap_addrect (region, rects, &first_rect,
                                                rx1, h, base + ib, h + 1);
                        if (rects == NULL)
                            goto error;
                        in_box = FALSE;
                    }
                }
                
                w = SCREEN_SHIFT_LEFT(w, 1);
            }
        }

        if (width & 31)
        {
            
             w = READ(pw++);
            for (ib = 0; ib < (width & 31); ib++)
            {
                

                if (w & mask0)
                {
                    if (!in_box)
                    {
                        rx1 = base + ib;
                        
                        in_box = TRUE;
                    }
                }
                else
                {
                    if (in_box)
                    {
                        
                        rects = bitmap_addrect(region, rects, &first_rect,
					       rx1, h, base + ib, h + 1);
			if (rects == NULL)
			    goto error;
                        in_box = FALSE;
                    }
                }
                
                w = SCREEN_SHIFT_LEFT(w, 1);
            }
        }
        
        if (in_box)
        {
            rects = bitmap_addrect(region, rects, &first_rect,
				   rx1, h, base + (width & 31), h + 1);
	    if (rects == NULL)
		goto error;
        }
        



        same = FALSE;
        if (irect_prev_start != -1)
        {
            crects = irect_line_start - irect_prev_start;
            if (crects != 0 &&
                crects == ((rects - first_rect) - irect_line_start))
            {
                old_rect = first_rect + irect_prev_start;
                new_rect = prect_line_start = first_rect + irect_line_start;
                same = TRUE;
                while (old_rect < prect_line_start)
                {
                    if ((old_rect->x1 != new_rect->x1) ||
                        (old_rect->x2 != new_rect->x2))
                    {
                          same = FALSE;
                          break;
                    }
                    old_rect++;
                    new_rect++;
                }
                if (same)
                {
                    old_rect = first_rect + irect_prev_start;
                    while (old_rect < prect_line_start)
                    {
                        old_rect->y2 += 1;
                        old_rect++;
                    }
                    rects -= crects;
                    region->data->numRects -= crects;
                }
            }
        }
        if(!same)
            irect_prev_start = irect_line_start;
    }
    if (!region->data->numRects)
    {
        region->extents.x1 = region->extents.x2 = 0;
    }
    else
    {
        region->extents.y1 = PIXREGION_BOXPTR(region)->y1;
        region->extents.y2 = PIXREGION_END(region)->y2;
        if (region->data->numRects == 1)
        {
            free (region->data);
            region->data = NULL;
        }
    }

 error:
    return;
}
