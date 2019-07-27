














#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"







#if 0                           

const int jpeg_zigzag_order[DCTSIZE2] = {
   0,  1,  5,  6, 14, 15, 27, 28,
   2,  4,  7, 13, 16, 26, 29, 42,
   3,  8, 12, 17, 25, 30, 41, 43,
   9, 11, 18, 24, 31, 40, 44, 53,
  10, 19, 23, 32, 39, 45, 52, 54,
  20, 22, 33, 38, 46, 51, 55, 60,
  21, 34, 37, 47, 50, 56, 59, 61,
  35, 36, 48, 49, 57, 58, 62, 63
};

#endif















const int jpeg_natural_order[DCTSIZE2+16] = {
  0,  1,  8, 16,  9,  2,  3, 10,
 17, 24, 32, 25, 18, 11,  4,  5,
 12, 19, 26, 33, 40, 48, 41, 34,
 27, 20, 13,  6,  7, 14, 21, 28,
 35, 42, 49, 56, 57, 50, 43, 36,
 29, 22, 15, 23, 30, 37, 44, 51,
 58, 59, 52, 45, 38, 31, 39, 46,
 53, 60, 61, 54, 47, 55, 62, 63,
 63, 63, 63, 63, 63, 63, 63, 63, 
 63, 63, 63, 63, 63, 63, 63, 63
};






GLOBAL(long)
jdiv_round_up (long a, long b)


{
  return (a + b - 1L) / b;
}


GLOBAL(long)
jround_up (long a, long b)


{
  a += b - 1L;
  return a - (a % b);
}


GLOBAL(void)
jcopy_sample_rows (JSAMPARRAY input_array, int source_row,
                   JSAMPARRAY output_array, int dest_row,
                   int num_rows, JDIMENSION num_cols)





{
  register JSAMPROW inptr, outptr;
  register size_t count = (size_t) (num_cols * sizeof(JSAMPLE));
  register int row;

  input_array += source_row;
  output_array += dest_row;

  for (row = num_rows; row > 0; row--) {
    inptr = *input_array++;
    outptr = *output_array++;
    MEMCOPY(outptr, inptr, count);
  }
}


GLOBAL(void)
jcopy_block_row (JBLOCKROW input_row, JBLOCKROW output_row,
                 JDIMENSION num_blocks)

{
  MEMCOPY(output_row, input_row, num_blocks * (DCTSIZE2 * sizeof(JCOEF)));
}


GLOBAL(void)
jzero_far (void * target, size_t bytestozero)


{
  MEMZERO(target, bytestozero);
}
