

























#define JPEG_INTERNALS
#define AM_MEMORY_MANAGER
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"		

#ifndef NO_GETENV
#ifndef HAVE_STDLIB_H		
extern char * getenv JPP((const char * name));
#endif
#endif


































#ifndef ALIGN_SIZE		
#ifndef WITH_SIMD
#define ALIGN_SIZE  SIZEOF(double)
#else
#define ALIGN_SIZE  16 /* Most SIMD implementations require this */
#endif
#endif










typedef struct small_pool_struct * small_pool_ptr;

typedef struct small_pool_struct {
  small_pool_ptr next;	
  size_t bytes_used;		
  size_t bytes_left;		
} small_pool_hdr;

typedef struct large_pool_struct FAR * large_pool_ptr;

typedef struct large_pool_struct {
  large_pool_ptr next;	
  size_t bytes_used;		
  size_t bytes_left;		
} large_pool_hdr;





typedef struct {
  struct jpeg_memory_mgr pub;	

  
  small_pool_ptr small_list[JPOOL_NUMPOOLS];
  large_pool_ptr large_list[JPOOL_NUMPOOLS];

  




  jvirt_sarray_ptr virt_sarray_list;
  jvirt_barray_ptr virt_barray_list;

  
  size_t total_space_allocated;

  


  JDIMENSION last_rowsperchunk;	
} my_memory_mgr;

typedef my_memory_mgr * my_mem_ptr;









struct jvirt_sarray_control {
  JSAMPARRAY mem_buffer;	
  JDIMENSION rows_in_array;	
  JDIMENSION samplesperrow;	
  JDIMENSION maxaccess;		
  JDIMENSION rows_in_mem;	
  JDIMENSION rowsperchunk;	
  JDIMENSION cur_start_row;	
  JDIMENSION first_undef_row;	
  boolean pre_zero;		
  boolean dirty;		
  boolean b_s_open;		
  jvirt_sarray_ptr next;	
  backing_store_info b_s_info;	
};

struct jvirt_barray_control {
  JBLOCKARRAY mem_buffer;	
  JDIMENSION rows_in_array;	
  JDIMENSION blocksperrow;	
  JDIMENSION maxaccess;		
  JDIMENSION rows_in_mem;	
  JDIMENSION rowsperchunk;	
  JDIMENSION cur_start_row;	
  JDIMENSION first_undef_row;	
  boolean pre_zero;		
  boolean dirty;		
  boolean b_s_open;		
  jvirt_barray_ptr next;	
  backing_store_info b_s_info;	
};


#ifdef MEM_STATS		

LOCAL(void)
print_mem_stats (j_common_ptr cinfo, int pool_id)
{
  my_mem_ptr mem = (my_mem_ptr) cinfo->mem;
  small_pool_ptr shdr_ptr;
  large_pool_ptr lhdr_ptr;

  



  fprintf(stderr, "Freeing pool %d, total space = %ld\n",
	  pool_id, mem->total_space_allocated);

  for (lhdr_ptr = mem->large_list[pool_id]; lhdr_ptr != NULL;
       lhdr_ptr = lhdr_ptr->next) {
    fprintf(stderr, "  Large chunk used %ld\n",
	    (long) lhdr_ptr->bytes_used);
  }

  for (shdr_ptr = mem->small_list[pool_id]; shdr_ptr != NULL;
       shdr_ptr = shdr_ptr->next) {
    fprintf(stderr, "  Small chunk used %ld free %ld\n",
	    (long) shdr_ptr->bytes_used,
	    (long) shdr_ptr->bytes_left);
  }
}

#endif 


LOCAL(void)
out_of_memory (j_common_ptr cinfo, int which)


{
#ifdef MEM_STATS
  cinfo->err->trace_level = 2;	
#endif
  ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, which);
}



















static const size_t first_pool_slop[JPOOL_NUMPOOLS] = 
{
	1600,			
	16000			
};

static const size_t extra_pool_slop[JPOOL_NUMPOOLS] = 
{
	0,			
	5000			
};

#define MIN_SLOP  50		/* greater than 0 to avoid futile looping */


METHODDEF(void *)
alloc_small (j_common_ptr cinfo, int pool_id, size_t sizeofobject)

{
  my_mem_ptr mem = (my_mem_ptr) cinfo->mem;
  small_pool_ptr hdr_ptr, prev_hdr_ptr;
  char * data_ptr;
  size_t min_request, slop;

  





  sizeofobject = jround_up(sizeofobject, ALIGN_SIZE);

  
  if ((SIZEOF(small_pool_hdr) + sizeofobject + ALIGN_SIZE - 1) > MAX_ALLOC_CHUNK)
    out_of_memory(cinfo, 1);	

  
  if (pool_id < 0 || pool_id >= JPOOL_NUMPOOLS)
    ERREXIT1(cinfo, JERR_BAD_POOL_ID, pool_id);	
  prev_hdr_ptr = NULL;
  hdr_ptr = mem->small_list[pool_id];
  while (hdr_ptr != NULL) {
    if (hdr_ptr->bytes_left >= sizeofobject)
      break;			
    prev_hdr_ptr = hdr_ptr;
    hdr_ptr = hdr_ptr->next;
  }

  
  if (hdr_ptr == NULL) {
    
    min_request = SIZEOF(small_pool_hdr) + sizeofobject + ALIGN_SIZE - 1;
    if (prev_hdr_ptr == NULL)	
      slop = first_pool_slop[pool_id];
    else
      slop = extra_pool_slop[pool_id];
    
    if (slop > (size_t) (MAX_ALLOC_CHUNK-min_request))
      slop = (size_t) (MAX_ALLOC_CHUNK-min_request);
    
    for (;;) {
      hdr_ptr = (small_pool_ptr) jpeg_get_small(cinfo, min_request + slop);
      if (hdr_ptr != NULL)
	break;
      slop /= 2;
      if (slop < MIN_SLOP)	
	out_of_memory(cinfo, 2); 
    }
    mem->total_space_allocated += min_request + slop;
    
    hdr_ptr->next = NULL;
    hdr_ptr->bytes_used = 0;
    hdr_ptr->bytes_left = sizeofobject + slop;
    if (prev_hdr_ptr == NULL)	
      mem->small_list[pool_id] = hdr_ptr;
    else
      prev_hdr_ptr->next = hdr_ptr;
  }

  
  data_ptr = (char *) hdr_ptr; 
  data_ptr += SIZEOF(small_pool_hdr); 
  if ((size_t)data_ptr % ALIGN_SIZE) 
    data_ptr += ALIGN_SIZE - (size_t)data_ptr % ALIGN_SIZE;
  data_ptr += hdr_ptr->bytes_used; 
  hdr_ptr->bytes_used += sizeofobject;
  hdr_ptr->bytes_left -= sizeofobject;

  return (void *) data_ptr;
}
















METHODDEF(void FAR *)
alloc_large (j_common_ptr cinfo, int pool_id, size_t sizeofobject)

{
  my_mem_ptr mem = (my_mem_ptr) cinfo->mem;
  large_pool_ptr hdr_ptr;
  char FAR * data_ptr;

  




  sizeofobject = jround_up(sizeofobject, ALIGN_SIZE);

  
  if ((SIZEOF(large_pool_hdr) + sizeofobject + ALIGN_SIZE - 1) > MAX_ALLOC_CHUNK)
    out_of_memory(cinfo, 3);	

  
  if (pool_id < 0 || pool_id >= JPOOL_NUMPOOLS)
    ERREXIT1(cinfo, JERR_BAD_POOL_ID, pool_id);	

  hdr_ptr = (large_pool_ptr) jpeg_get_large(cinfo, sizeofobject +
					    SIZEOF(large_pool_hdr) +
					    ALIGN_SIZE - 1);
  if (hdr_ptr == NULL)
    out_of_memory(cinfo, 4);	
  mem->total_space_allocated += sizeofobject + SIZEOF(large_pool_hdr) + ALIGN_SIZE - 1;

  
  hdr_ptr->next = mem->large_list[pool_id];
  


  hdr_ptr->bytes_used = sizeofobject;
  hdr_ptr->bytes_left = 0;
  mem->large_list[pool_id] = hdr_ptr;

  data_ptr = (char *) hdr_ptr; 
  data_ptr += SIZEOF(small_pool_hdr); 
  if ((size_t)data_ptr % ALIGN_SIZE) 
    data_ptr += ALIGN_SIZE - (size_t)data_ptr % ALIGN_SIZE;

  return (void FAR *) data_ptr;
}



















METHODDEF(JSAMPARRAY)
alloc_sarray (j_common_ptr cinfo, int pool_id,
	      JDIMENSION samplesperrow, JDIMENSION numrows)

{
  my_mem_ptr mem = (my_mem_ptr) cinfo->mem;
  JSAMPARRAY result;
  JSAMPROW workspace;
  JDIMENSION rowsperchunk, currow, i;
  long ltemp;

  
  if ((ALIGN_SIZE % SIZEOF(JSAMPLE)) != 0)
    out_of_memory(cinfo, 5);	
  samplesperrow = (JDIMENSION)jround_up(samplesperrow, (2 * ALIGN_SIZE) / SIZEOF(JSAMPLE));

  
  ltemp = (MAX_ALLOC_CHUNK-SIZEOF(large_pool_hdr)) /
	  ((long) samplesperrow * SIZEOF(JSAMPLE));
  if (ltemp <= 0)
    ERREXIT(cinfo, JERR_WIDTH_OVERFLOW);
  if (ltemp < (long) numrows)
    rowsperchunk = (JDIMENSION) ltemp;
  else
    rowsperchunk = numrows;
  mem->last_rowsperchunk = rowsperchunk;

  
  result = (JSAMPARRAY) alloc_small(cinfo, pool_id,
				    (size_t) (numrows * SIZEOF(JSAMPROW)));

  
  currow = 0;
  while (currow < numrows) {
    rowsperchunk = MIN(rowsperchunk, numrows - currow);
    workspace = (JSAMPROW) alloc_large(cinfo, pool_id,
	(size_t) ((size_t) rowsperchunk * (size_t) samplesperrow
		  * SIZEOF(JSAMPLE)));
    for (i = rowsperchunk; i > 0; i--) {
      result[currow++] = workspace;
      workspace += samplesperrow;
    }
  }

  return result;
}







METHODDEF(JBLOCKARRAY)
alloc_barray (j_common_ptr cinfo, int pool_id,
	      JDIMENSION blocksperrow, JDIMENSION numrows)

{
  my_mem_ptr mem = (my_mem_ptr) cinfo->mem;
  JBLOCKARRAY result;
  JBLOCKROW workspace;
  JDIMENSION rowsperchunk, currow, i;
  long ltemp;

  
  if ((SIZEOF(JBLOCK) % ALIGN_SIZE) != 0)
    out_of_memory(cinfo, 6);	

  
  ltemp = (MAX_ALLOC_CHUNK-SIZEOF(large_pool_hdr)) /
	  ((long) blocksperrow * SIZEOF(JBLOCK));
  if (ltemp <= 0)
    ERREXIT(cinfo, JERR_WIDTH_OVERFLOW);
  if (ltemp < (long) numrows)
    rowsperchunk = (JDIMENSION) ltemp;
  else
    rowsperchunk = numrows;
  mem->last_rowsperchunk = rowsperchunk;

  
  result = (JBLOCKARRAY) alloc_small(cinfo, pool_id,
				     (size_t) (numrows * SIZEOF(JBLOCKROW)));

  
  currow = 0;
  while (currow < numrows) {
    rowsperchunk = MIN(rowsperchunk, numrows - currow);
    workspace = (JBLOCKROW) alloc_large(cinfo, pool_id,
	(size_t) ((size_t) rowsperchunk * (size_t) blocksperrow
		  * SIZEOF(JBLOCK)));
    for (i = rowsperchunk; i > 0; i--) {
      result[currow++] = workspace;
      workspace += blocksperrow;
    }
  }

  return result;
}







































METHODDEF(jvirt_sarray_ptr)
request_virt_sarray (j_common_ptr cinfo, int pool_id, boolean pre_zero,
		     JDIMENSION samplesperrow, JDIMENSION numrows,
		     JDIMENSION maxaccess)

{
  my_mem_ptr mem = (my_mem_ptr) cinfo->mem;
  jvirt_sarray_ptr result;

  
  if (pool_id != JPOOL_IMAGE)
    ERREXIT1(cinfo, JERR_BAD_POOL_ID, pool_id);	

  
  result = (jvirt_sarray_ptr) alloc_small(cinfo, pool_id,
					  SIZEOF(struct jvirt_sarray_control));

  result->mem_buffer = NULL;	
  result->rows_in_array = numrows;
  result->samplesperrow = samplesperrow;
  result->maxaccess = maxaccess;
  result->pre_zero = pre_zero;
  result->b_s_open = FALSE;	
  result->next = mem->virt_sarray_list; 
  mem->virt_sarray_list = result;

  return result;
}


METHODDEF(jvirt_barray_ptr)
request_virt_barray (j_common_ptr cinfo, int pool_id, boolean pre_zero,
		     JDIMENSION blocksperrow, JDIMENSION numrows,
		     JDIMENSION maxaccess)

{
  my_mem_ptr mem = (my_mem_ptr) cinfo->mem;
  jvirt_barray_ptr result;

  
  if (pool_id != JPOOL_IMAGE)
    ERREXIT1(cinfo, JERR_BAD_POOL_ID, pool_id);	

  
  result = (jvirt_barray_ptr) alloc_small(cinfo, pool_id,
					  SIZEOF(struct jvirt_barray_control));

  result->mem_buffer = NULL;	
  result->rows_in_array = numrows;
  result->blocksperrow = blocksperrow;
  result->maxaccess = maxaccess;
  result->pre_zero = pre_zero;
  result->b_s_open = FALSE;	
  result->next = mem->virt_barray_list; 
  mem->virt_barray_list = result;

  return result;
}


METHODDEF(void)
realize_virt_arrays (j_common_ptr cinfo)

{
  my_mem_ptr mem = (my_mem_ptr) cinfo->mem;
  size_t space_per_minheight, maximum_space, avail_mem;
  size_t minheights, max_minheights;
  jvirt_sarray_ptr sptr;
  jvirt_barray_ptr bptr;

  



  space_per_minheight = 0;
  maximum_space = 0;
  for (sptr = mem->virt_sarray_list; sptr != NULL; sptr = sptr->next) {
    if (sptr->mem_buffer == NULL) { 
      space_per_minheight += (long) sptr->maxaccess *
			     (long) sptr->samplesperrow * SIZEOF(JSAMPLE);
      maximum_space += (long) sptr->rows_in_array *
		       (long) sptr->samplesperrow * SIZEOF(JSAMPLE);
    }
  }
  for (bptr = mem->virt_barray_list; bptr != NULL; bptr = bptr->next) {
    if (bptr->mem_buffer == NULL) { 
      space_per_minheight += (long) bptr->maxaccess *
			     (long) bptr->blocksperrow * SIZEOF(JBLOCK);
      maximum_space += (long) bptr->rows_in_array *
		       (long) bptr->blocksperrow * SIZEOF(JBLOCK);
    }
  }

  if (space_per_minheight <= 0)
    return;			

  
  avail_mem = jpeg_mem_available(cinfo, space_per_minheight, maximum_space,
				 mem->total_space_allocated);

  



  if (avail_mem >= maximum_space)
    max_minheights = 1000000000L;
  else {
    max_minheights = avail_mem / space_per_minheight;
    


    if (max_minheights <= 0)
      max_minheights = 1;
  }

  

  for (sptr = mem->virt_sarray_list; sptr != NULL; sptr = sptr->next) {
    if (sptr->mem_buffer == NULL) { 
      minheights = ((long) sptr->rows_in_array - 1L) / sptr->maxaccess + 1L;
      if (minheights <= max_minheights) {
	
	sptr->rows_in_mem = sptr->rows_in_array;
      } else {
	
	sptr->rows_in_mem = (JDIMENSION) (max_minheights * sptr->maxaccess);
	jpeg_open_backing_store(cinfo, & sptr->b_s_info,
				(long) sptr->rows_in_array *
				(long) sptr->samplesperrow *
				(long) SIZEOF(JSAMPLE));
	sptr->b_s_open = TRUE;
      }
      sptr->mem_buffer = alloc_sarray(cinfo, JPOOL_IMAGE,
				      sptr->samplesperrow, sptr->rows_in_mem);
      sptr->rowsperchunk = mem->last_rowsperchunk;
      sptr->cur_start_row = 0;
      sptr->first_undef_row = 0;
      sptr->dirty = FALSE;
    }
  }

  for (bptr = mem->virt_barray_list; bptr != NULL; bptr = bptr->next) {
    if (bptr->mem_buffer == NULL) { 
      minheights = ((long) bptr->rows_in_array - 1L) / bptr->maxaccess + 1L;
      if (minheights <= max_minheights) {
	
	bptr->rows_in_mem = bptr->rows_in_array;
      } else {
	
	bptr->rows_in_mem = (JDIMENSION) (max_minheights * bptr->maxaccess);
	jpeg_open_backing_store(cinfo, & bptr->b_s_info,
				(long) bptr->rows_in_array *
				(long) bptr->blocksperrow *
				(long) SIZEOF(JBLOCK));
	bptr->b_s_open = TRUE;
      }
      bptr->mem_buffer = alloc_barray(cinfo, JPOOL_IMAGE,
				      bptr->blocksperrow, bptr->rows_in_mem);
      bptr->rowsperchunk = mem->last_rowsperchunk;
      bptr->cur_start_row = 0;
      bptr->first_undef_row = 0;
      bptr->dirty = FALSE;
    }
  }
}


LOCAL(void)
do_sarray_io (j_common_ptr cinfo, jvirt_sarray_ptr ptr, boolean writing)

{
  long bytesperrow, file_offset, byte_count, rows, thisrow, i;

  bytesperrow = (long) ptr->samplesperrow * SIZEOF(JSAMPLE);
  file_offset = ptr->cur_start_row * bytesperrow;
  
  for (i = 0; i < (long) ptr->rows_in_mem; i += ptr->rowsperchunk) {
    
    rows = MIN((long) ptr->rowsperchunk, (long) ptr->rows_in_mem - i);
    
    thisrow = (long) ptr->cur_start_row + i;
    rows = MIN(rows, (long) ptr->first_undef_row - thisrow);
    
    rows = MIN(rows, (long) ptr->rows_in_array - thisrow);
    if (rows <= 0)		
      break;
    byte_count = rows * bytesperrow;
    if (writing)
      (*ptr->b_s_info.write_backing_store) (cinfo, & ptr->b_s_info,
					    (void FAR *) ptr->mem_buffer[i],
					    file_offset, byte_count);
    else
      (*ptr->b_s_info.read_backing_store) (cinfo, & ptr->b_s_info,
					   (void FAR *) ptr->mem_buffer[i],
					   file_offset, byte_count);
    file_offset += byte_count;
  }
}


LOCAL(void)
do_barray_io (j_common_ptr cinfo, jvirt_barray_ptr ptr, boolean writing)

{
  long bytesperrow, file_offset, byte_count, rows, thisrow, i;

  bytesperrow = (long) ptr->blocksperrow * SIZEOF(JBLOCK);
  file_offset = ptr->cur_start_row * bytesperrow;
  
  for (i = 0; i < (long) ptr->rows_in_mem; i += ptr->rowsperchunk) {
    
    rows = MIN((long) ptr->rowsperchunk, (long) ptr->rows_in_mem - i);
    
    thisrow = (long) ptr->cur_start_row + i;
    rows = MIN(rows, (long) ptr->first_undef_row - thisrow);
    
    rows = MIN(rows, (long) ptr->rows_in_array - thisrow);
    if (rows <= 0)		
      break;
    byte_count = rows * bytesperrow;
    if (writing)
      (*ptr->b_s_info.write_backing_store) (cinfo, & ptr->b_s_info,
					    (void FAR *) ptr->mem_buffer[i],
					    file_offset, byte_count);
    else
      (*ptr->b_s_info.read_backing_store) (cinfo, & ptr->b_s_info,
					   (void FAR *) ptr->mem_buffer[i],
					   file_offset, byte_count);
    file_offset += byte_count;
  }
}


METHODDEF(JSAMPARRAY)
access_virt_sarray (j_common_ptr cinfo, jvirt_sarray_ptr ptr,
		    JDIMENSION start_row, JDIMENSION num_rows,
		    boolean writable)



{
  JDIMENSION end_row = start_row + num_rows;
  JDIMENSION undef_row;

  
  if (end_row > ptr->rows_in_array || num_rows > ptr->maxaccess ||
      ptr->mem_buffer == NULL)
    ERREXIT(cinfo, JERR_BAD_VIRTUAL_ACCESS);

  
  if (start_row < ptr->cur_start_row ||
      end_row > ptr->cur_start_row+ptr->rows_in_mem) {
    if (! ptr->b_s_open)
      ERREXIT(cinfo, JERR_VIRTUAL_BUG);
    
    if (ptr->dirty) {
      do_sarray_io(cinfo, ptr, TRUE);
      ptr->dirty = FALSE;
    }
    






    if (start_row > ptr->cur_start_row) {
      ptr->cur_start_row = start_row;
    } else {
      
      long ltemp;

      ltemp = (long) end_row - (long) ptr->rows_in_mem;
      if (ltemp < 0)
	ltemp = 0;		
      ptr->cur_start_row = (JDIMENSION) ltemp;
    }
    



    do_sarray_io(cinfo, ptr, FALSE);
  }
  



  if (ptr->first_undef_row < end_row) {
    if (ptr->first_undef_row < start_row) {
      if (writable)		
	ERREXIT(cinfo, JERR_BAD_VIRTUAL_ACCESS);
      undef_row = start_row;	
    } else {
      undef_row = ptr->first_undef_row;
    }
    if (writable)
      ptr->first_undef_row = end_row;
    if (ptr->pre_zero) {
      size_t bytesperrow = (size_t) ptr->samplesperrow * SIZEOF(JSAMPLE);
      undef_row -= ptr->cur_start_row; 
      end_row -= ptr->cur_start_row;
      while (undef_row < end_row) {
	jzero_far((void FAR *) ptr->mem_buffer[undef_row], bytesperrow);
	undef_row++;
      }
    } else {
      if (! writable)		
	ERREXIT(cinfo, JERR_BAD_VIRTUAL_ACCESS);
    }
  }
  
  if (writable)
    ptr->dirty = TRUE;
  
  return ptr->mem_buffer + (start_row - ptr->cur_start_row);
}


METHODDEF(JBLOCKARRAY)
access_virt_barray (j_common_ptr cinfo, jvirt_barray_ptr ptr,
		    JDIMENSION start_row, JDIMENSION num_rows,
		    boolean writable)



{
  JDIMENSION end_row = start_row + num_rows;
  JDIMENSION undef_row;

  
  if (end_row > ptr->rows_in_array || num_rows > ptr->maxaccess ||
      ptr->mem_buffer == NULL)
    ERREXIT(cinfo, JERR_BAD_VIRTUAL_ACCESS);

  
  if (start_row < ptr->cur_start_row ||
      end_row > ptr->cur_start_row+ptr->rows_in_mem) {
    if (! ptr->b_s_open)
      ERREXIT(cinfo, JERR_VIRTUAL_BUG);
    
    if (ptr->dirty) {
      do_barray_io(cinfo, ptr, TRUE);
      ptr->dirty = FALSE;
    }
    






    if (start_row > ptr->cur_start_row) {
      ptr->cur_start_row = start_row;
    } else {
      
      long ltemp;

      ltemp = (long) end_row - (long) ptr->rows_in_mem;
      if (ltemp < 0)
	ltemp = 0;		
      ptr->cur_start_row = (JDIMENSION) ltemp;
    }
    



    do_barray_io(cinfo, ptr, FALSE);
  }
  



  if (ptr->first_undef_row < end_row) {
    if (ptr->first_undef_row < start_row) {
      if (writable)		
	ERREXIT(cinfo, JERR_BAD_VIRTUAL_ACCESS);
      undef_row = start_row;	
    } else {
      undef_row = ptr->first_undef_row;
    }
    if (writable)
      ptr->first_undef_row = end_row;
    if (ptr->pre_zero) {
      size_t bytesperrow = (size_t) ptr->blocksperrow * SIZEOF(JBLOCK);
      undef_row -= ptr->cur_start_row; 
      end_row -= ptr->cur_start_row;
      while (undef_row < end_row) {
	jzero_far((void FAR *) ptr->mem_buffer[undef_row], bytesperrow);
	undef_row++;
      }
    } else {
      if (! writable)		
	ERREXIT(cinfo, JERR_BAD_VIRTUAL_ACCESS);
    }
  }
  
  if (writable)
    ptr->dirty = TRUE;
  
  return ptr->mem_buffer + (start_row - ptr->cur_start_row);
}






METHODDEF(void)
free_pool (j_common_ptr cinfo, int pool_id)
{
  my_mem_ptr mem = (my_mem_ptr) cinfo->mem;
  small_pool_ptr shdr_ptr;
  large_pool_ptr lhdr_ptr;
  size_t space_freed;

  if (pool_id < 0 || pool_id >= JPOOL_NUMPOOLS)
    ERREXIT1(cinfo, JERR_BAD_POOL_ID, pool_id);	

#ifdef MEM_STATS
  if (cinfo->err->trace_level > 1)
    print_mem_stats(cinfo, pool_id); 
#endif

  
  if (pool_id == JPOOL_IMAGE) {
    jvirt_sarray_ptr sptr;
    jvirt_barray_ptr bptr;

    for (sptr = mem->virt_sarray_list; sptr != NULL; sptr = sptr->next) {
      if (sptr->b_s_open) {	
	sptr->b_s_open = FALSE;	
	(*sptr->b_s_info.close_backing_store) (cinfo, & sptr->b_s_info);
      }
    }
    mem->virt_sarray_list = NULL;
    for (bptr = mem->virt_barray_list; bptr != NULL; bptr = bptr->next) {
      if (bptr->b_s_open) {	
	bptr->b_s_open = FALSE;	
	(*bptr->b_s_info.close_backing_store) (cinfo, & bptr->b_s_info);
      }
    }
    mem->virt_barray_list = NULL;
  }

  
  lhdr_ptr = mem->large_list[pool_id];
  mem->large_list[pool_id] = NULL;

  while (lhdr_ptr != NULL) {
    large_pool_ptr next_lhdr_ptr = lhdr_ptr->next;
    space_freed = lhdr_ptr->bytes_used +
		  lhdr_ptr->bytes_left +
		  SIZEOF(large_pool_hdr);
    jpeg_free_large(cinfo, (void FAR *) lhdr_ptr, space_freed);
    mem->total_space_allocated -= space_freed;
    lhdr_ptr = next_lhdr_ptr;
  }

  
  shdr_ptr = mem->small_list[pool_id];
  mem->small_list[pool_id] = NULL;

  while (shdr_ptr != NULL) {
    small_pool_ptr next_shdr_ptr = shdr_ptr->next;
    space_freed = shdr_ptr->bytes_used +
		  shdr_ptr->bytes_left +
		  SIZEOF(small_pool_hdr);
    jpeg_free_small(cinfo, (void *) shdr_ptr, space_freed);
    mem->total_space_allocated -= space_freed;
    shdr_ptr = next_shdr_ptr;
  }
}







METHODDEF(void)
self_destruct (j_common_ptr cinfo)
{
  int pool;

  



  for (pool = JPOOL_NUMPOOLS-1; pool >= JPOOL_PERMANENT; pool--) {
    free_pool(cinfo, pool);
  }

  
  jpeg_free_small(cinfo, (void *) cinfo->mem, SIZEOF(my_memory_mgr));
  cinfo->mem = NULL;		

  jpeg_mem_term(cinfo);		
}







GLOBAL(void)
jinit_memory_mgr (j_common_ptr cinfo)
{
  my_mem_ptr mem;
  long max_to_use;
  int pool;
  size_t test_mac;

  cinfo->mem = NULL;		

  






  if ((ALIGN_SIZE & (ALIGN_SIZE-1)) != 0)
    ERREXIT(cinfo, JERR_BAD_ALIGN_TYPE);
  




  test_mac = (size_t) MAX_ALLOC_CHUNK;
  if ((long) test_mac != MAX_ALLOC_CHUNK ||
      (MAX_ALLOC_CHUNK % ALIGN_SIZE) != 0)
    ERREXIT(cinfo, JERR_BAD_ALLOC_CHUNK);

  max_to_use = jpeg_mem_init(cinfo); 

  
  mem = (my_mem_ptr) jpeg_get_small(cinfo, SIZEOF(my_memory_mgr));

  if (mem == NULL) {
    jpeg_mem_term(cinfo);	
    ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, 0);
  }

  
  mem->pub.alloc_small = alloc_small;
  mem->pub.alloc_large = alloc_large;
  mem->pub.alloc_sarray = alloc_sarray;
  mem->pub.alloc_barray = alloc_barray;
  mem->pub.request_virt_sarray = request_virt_sarray;
  mem->pub.request_virt_barray = request_virt_barray;
  mem->pub.realize_virt_arrays = realize_virt_arrays;
  mem->pub.access_virt_sarray = access_virt_sarray;
  mem->pub.access_virt_barray = access_virt_barray;
  mem->pub.free_pool = free_pool;
  mem->pub.self_destruct = self_destruct;

  
  mem->pub.max_alloc_chunk = MAX_ALLOC_CHUNK;

  
  mem->pub.max_memory_to_use = max_to_use;

  for (pool = JPOOL_NUMPOOLS-1; pool >= JPOOL_PERMANENT; pool--) {
    mem->small_list[pool] = NULL;
    mem->large_list[pool] = NULL;
  }
  mem->virt_sarray_list = NULL;
  mem->virt_barray_list = NULL;

  mem->total_space_allocated = SIZEOF(my_memory_mgr);

  
  cinfo->mem = & mem->pub;

  





#ifndef NO_GETENV
  { char * memenv;

    if ((memenv = getenv("JPEGMEM")) != NULL) {
      char ch = 'x';

      if (sscanf(memenv, "%ld%c", &max_to_use, &ch) > 0) {
	if (ch == 'm' || ch == 'M')
	  max_to_use *= 1000L;
	mem->pub.max_memory_to_use = max_to_use * 1000L;
      }
    }
  }
#endif

}
