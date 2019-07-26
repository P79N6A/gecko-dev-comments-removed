






#ifdef ENABLE_PARALLEL_JS


#define TRY_PARALLEL(MODE) \
  ((!MODE || MODE.mode !== "seq"))
#define ASSERT_SEQUENTIAL_IS_OK(MODE) \
  do { if (MODE) AssertSequentialIsOK(MODE) } while(false)





#ifndef DEBUG
#define ParallelSpew(args)
#endif

#define MAX_SLICE_SHIFT 6
#define MAX_SLICE_SIZE 64
#define MAX_SLICES_PER_WORKER 8





#define SLICE_START_INDEX(shift, id) \
    (id << shift)
#define SLICE_END_INDEX(shift, start, length) \
    std_Math_min(start + (1 << shift), length)







#define GET_SLICE(sliceStart, sliceEnd, id) \
    ((id = ForkJoinGetSlice((InParallelSection() ? -1 : sliceStart++) | 0)) < sliceEnd)










function ComputeSlicesInfo(length) {
  var count = length >>> MAX_SLICE_SHIFT;
  var numWorkers = ForkJoinNumWorkers();
  if (count < numWorkers)
    count = numWorkers;
  else if (count >= numWorkers * MAX_SLICES_PER_WORKER)
    count = numWorkers * MAX_SLICES_PER_WORKER;

  
  var shift = std_Math_max(std_Math_log2(length / count) | 0, 1);

  
  count = length >>> shift;
  if (count << shift !== length)
    count += 1;

  return { shift: shift, count: count };
}

#endif 
