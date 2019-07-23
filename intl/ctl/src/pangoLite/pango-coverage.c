







































#include <string.h>

#include "pango-coverage.h"

typedef struct _PangoliteBlockInfo PangoliteBlockInfo;

#define N_BLOCKS_INCREMENT 256






struct _PangoliteBlockInfo
{
  guchar *data;		
  PangoliteCoverageLevel level;	
};

struct _PangoliteCoverage
{
  guint ref_count;
  int n_blocks;
  int data_size;
  
  PangoliteBlockInfo *blocks;
};









PangoliteCoverage *
pangolite_coverage_new (void)
{
  PangoliteCoverage *coverage = g_new (PangoliteCoverage, 1);

  coverage->n_blocks = N_BLOCKS_INCREMENT;
  coverage->blocks = g_new0 (PangoliteBlockInfo, coverage->n_blocks);
  coverage->ref_count = 1;
  
  return coverage;
}











PangoliteCoverage *
pangolite_coverage_copy (PangoliteCoverage *coverage)
{
  int i;
  PangoliteCoverage *result;

  g_return_val_if_fail (coverage != NULL, NULL);

  result = g_new (PangoliteCoverage, 1);
  result->n_blocks = coverage->n_blocks;
  result->blocks = g_new (PangoliteBlockInfo, coverage->n_blocks);
  result->ref_count = 1;

  for (i=0; i<coverage->n_blocks; i++) {
    if (coverage->blocks[i].data) {
      result->blocks[i].data = g_new (guchar, 64);
      memcpy (result->blocks[i].data, coverage->blocks[i].data, 64);
    }
    else
      result->blocks[i].data = NULL;
    
    result->blocks[i].level = coverage->blocks[i].level;
  }
  
  return result;
}









PangoliteCoverage *
pangolite_coverage_ref (PangoliteCoverage *coverage)
{
  g_return_val_if_fail (coverage != NULL, NULL);

  coverage->ref_count++;
  return coverage;
}








void
pangolite_coverage_unref (PangoliteCoverage *coverage)
{
  int i;
  
  g_return_if_fail (coverage != NULL);
  g_return_if_fail (coverage->ref_count > 0);

  coverage->ref_count--;

  if (coverage->ref_count == 0) {
    for (i=0; i<coverage->n_blocks; i++) {
      if (coverage->blocks[i].data)
        g_free (coverage->blocks[i].data);
    }
    
    g_free (coverage->blocks);
    g_free (coverage);
  }
}










PangoliteCoverageLevel
pangolite_coverage_get (PangoliteCoverage *coverage, int index)
{
  int block_index;
  
  g_return_val_if_fail (coverage != NULL, PANGO_COVERAGE_NONE);

  block_index = index / 256;

  if (block_index > coverage->n_blocks)
    return PANGO_COVERAGE_NONE;
  else {
    guchar *data = coverage->blocks[block_index].data;
    if (data) {
      int i = index % 256;
      int shift = (i % 4) * 2;
      
      return (data[i/4] >> shift) & 0x3;
    }
    else
      return coverage->blocks[block_index].level;
  }
}









void pangolite_coverage_set (PangoliteCoverage     *coverage,
                         int                index,
                         PangoliteCoverageLevel level)
{
  int block_index, i;
  guchar *data;
  
  g_return_if_fail (coverage != NULL);
  g_return_if_fail (level >= 0 || level <= 3);

  block_index = index / 256;

  if (block_index > coverage->n_blocks) {
    int old_n_blocks = coverage->n_blocks;
    
    coverage->n_blocks =
      N_BLOCKS_INCREMENT * ((block_index + N_BLOCKS_INCREMENT - 1) / N_BLOCKS_INCREMENT);
    
    coverage->blocks = g_renew (PangoliteBlockInfo, coverage->blocks, coverage->n_blocks);
    memset (coverage->blocks + old_n_blocks, 0,
            sizeof (PangoliteBlockInfo) * (coverage->n_blocks - old_n_blocks));
  }
  
  data = coverage->blocks[block_index].data;
  if (!data) {
    guchar byte;
    
    if (level == coverage->blocks[block_index].level)
      return;
    
    data = g_new (guchar, 64);
    coverage->blocks[block_index].data = data;
    
    byte = coverage->blocks[block_index].level |
      (coverage->blocks[block_index].level << 2) |
      (coverage->blocks[block_index].level << 4) |
      (coverage->blocks[block_index].level << 6);
    
    memset (data, byte, 64);
  }
  
  i = index % 256;
  data[i/4] |= level << ((i % 4) * 2);
}










void pangolite_coverage_max (PangoliteCoverage *coverage, PangoliteCoverage *other)
{
  int block_index, i;
  int old_blocks;
  
  g_return_if_fail (coverage != NULL);
  
  old_blocks = MIN (coverage->n_blocks, other->n_blocks);
  
  if (other->n_blocks > coverage->n_blocks) {
    coverage->n_blocks = other->n_blocks;
    coverage->blocks = g_renew (PangoliteBlockInfo, coverage->blocks, coverage->n_blocks);
    
    for (block_index = old_blocks; block_index < coverage->n_blocks; 
         block_index++) {
      if (other->blocks[block_index].data) {
        coverage->blocks[block_index].data = g_new (guchar, 64);
	      memcpy (coverage->blocks[block_index].data, 
                other->blocks[block_index].data, 64);
	    }
      else
        coverage->blocks[block_index].data = NULL;
      
      coverage->blocks[block_index].level = other->blocks[block_index].level;
    }
  }
  
  for (block_index = 0; block_index < old_blocks; block_index++) {
    if (!coverage->blocks[block_index].data && !other->blocks[block_index].data) {
      coverage->blocks[block_index].level = MAX (coverage->blocks[block_index].level, other->blocks[block_index].level);
    }
    else if (coverage->blocks[block_index].data && other->blocks[block_index].data) {
      guchar *data = coverage->blocks[block_index].data;
      
      for (i=0; i<64; i++) {
	      int byte1 = data[i];
	      int byte2 = other->blocks[block_index].data[i];
        
	      
	      data[i] =
          MAX (byte1 & 0x3, byte2 & 0x3) |
          MAX (byte1 & 0xc, byte2 & 0xc) |
          MAX (byte1 & 0x30, byte2 & 0x30) |
          MAX (byte1 & 0xc0, byte2 & 0xc00);
	    }
    }
    else {
      guchar *src, *dest;
      int level, byte2;
      
      if (coverage->blocks[block_index].data) {
	      src = dest = coverage->blocks[block_index].data;
	      level = other->blocks[block_index].level;
	    }
      else {
	      src = other->blocks[block_index].data;
	      dest = g_new (guchar, 64);
	      coverage->blocks[block_index].data = dest;
	      level = coverage->blocks[block_index].level;
	    }
      
      byte2 = level | (level << 2) | (level << 4) | (level << 6);
      
      for (i=0; i<64; i++) {
	      int byte1 = src[i];
        
	      
	      dest[i] =
          MAX (byte1 & 0x3, byte2 & 0x3) |
          MAX (byte1 & 0xc, byte2 & 0xc) |
          MAX (byte1 & 0x30, byte2 & 0x30) |
          MAX (byte1 & 0xc0, byte2 & 0xc00);
	    }
    }
  }
}

#define PANGO_COVERAGE_MAGIC 0xc89dbd5e









void
pangolite_coverage_to_bytes(PangoliteCoverage  *coverage,
                        guchar        **bytes,
                        int            *n_bytes)
{
  int i, j;
  int size = 8 + 4 * coverage->n_blocks;
  guchar *data;
  int offset;
  
  for (i=0; i<coverage->n_blocks; i++) {
    if (coverage->blocks[i].data)
      size += 64;
  }
  
  data = g_malloc (size);
  
  *(guint32 *)&data[0] = g_htonl (PANGO_COVERAGE_MAGIC); 
  *(guint32 *)&data[4] = g_htonl (coverage->n_blocks);
  offset = 8;
  
  for (i=0; i<coverage->n_blocks; i++) {
    guint32 header_val;
    
    



    if (coverage->blocks[i].data != NULL) {
      guchar *data = coverage->blocks[i].data;
      guchar first_val = data[0];
      
      for (j = 1 ; j < 64; j++)
        if (data[j] != first_val)
          break;
      
      if (j == 64) {
	      g_free (data);
	      coverage->blocks[i].data = NULL;
	      coverage->blocks[i].level = first_val & 0x3;
	    }
    }
    
    if (coverage->blocks[i].data != NULL)
      header_val = (guint32)-1;
    else
      header_val = coverage->blocks[i].level;
    
    *(guint32 *)&data[offset] = g_htonl (header_val);
    offset += 4;
    
    if (coverage->blocks[i].data) {
      memcpy (data + offset, coverage->blocks[i].data, 64);
      offset += 64;
    }
  }
  
  *bytes = data;
  *n_bytes = size;
}

static guint32
pangolite_coverage_get_uint32 (guchar **ptr)
{
  guint32 val;
  
  memcpy (&val, *ptr, 4);
  *ptr += 4;
  
  return g_ntohl (val);
}












PangoliteCoverage *
pangolite_coverage_from_bytes (guchar *bytes, int n_bytes)
{
  PangoliteCoverage *coverage = g_new0 (PangoliteCoverage, 1);
  guchar *ptr = bytes;
  int i;
  
  coverage->ref_count = 1;
  
  if (n_bytes < 8)
    goto error;
  
  if (pangolite_coverage_get_uint32 (&ptr) != PANGO_COVERAGE_MAGIC)
    goto error;
  
  coverage->n_blocks = pangolite_coverage_get_uint32 (&ptr);
  coverage->blocks = g_new0 (PangoliteBlockInfo, coverage->n_blocks);
  
  for (i = 0; i < coverage->n_blocks; i++) {
    guint val;
    
    if (ptr + 4 > bytes + n_bytes)
      goto error;
    
    val = pangolite_coverage_get_uint32 (&ptr);
    if (val == (guint32)-1) {
      if (ptr + 64 > bytes + n_bytes)
        goto error;
      
      coverage->blocks[i].data = g_new (guchar, 64);
      memcpy (coverage->blocks[i].data, ptr, 64);
      ptr += 64;
    }
    else
      coverage->blocks[i].level = val;
  }
  
  return coverage;
  
 error:
  
  pangolite_coverage_unref (coverage);
  return NULL;
}
