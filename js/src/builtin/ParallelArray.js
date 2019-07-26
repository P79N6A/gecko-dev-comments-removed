






#define TRY_PARALLEL(MODE) \
  ((!MODE || MODE.mode !== "seq"))
#define ASSERT_SEQUENTIAL_IS_OK(MODE) \
  do { if (MODE) AssertSequentialIsOK(MODE) } while(false)


#define SLICE_INFO(START, END) START, END, START, 0
#define SLICE_START(ID) ((ID << 2) + 0)
#define SLICE_END(ID)   ((ID << 2) + 1)
#define SLICE_POS(ID)   ((ID << 2) + 2)




#define CHUNK_SHIFT 5
#define CHUNK_SIZE 32


#define ARRAY_PUSH(ARRAY, ELEMENT) \
  callFunction(std_Array_push, ARRAY, ELEMENT);
#define ARRAY_SLICE(ARRAY, ELEMENT) \
  callFunction(std_Array_slice, ARRAY, ELEMENT);





#ifndef DEBUG
#define ParallelSpew(args)
#endif





function ComputeNumChunks(length) {
  var chunks = length >>> CHUNK_SHIFT;
  if (chunks << CHUNK_SHIFT === length)
    return chunks;
  return chunks + 1;
}







function ComputeSliceBounds(numItems, sliceIndex, numSlices) {
  var sliceWidth = (numItems / numSlices) | 0;
  var startIndex = sliceWidth * sliceIndex;
  var endIndex = sliceIndex === numSlices - 1 ? numItems : sliceWidth * (sliceIndex + 1);
  return [startIndex, endIndex];
}









function ComputeAllSliceBounds(numItems, numSlices) {
  
  var info = [];
  for (var i = 0; i < numSlices; i++) {
    var [start, end] = ComputeSliceBounds(numItems, i, numSlices);
    ARRAY_PUSH(info, SLICE_INFO(start, end));
  }
  return info;
}






function ComputeProducts(shape) {
  var product = 1;
  var products = [1];
  var sdimensionality = shape.length;
  for (var i = sdimensionality - 1; i > 0; i--) {
    product *= shape[i];
    ARRAY_PUSH(products, product);
  }
  return products;
}





function ComputeIndices(shape, index1d) {

  var products = ComputeProducts(shape);
  var l = shape.length;

  var result = [];
  for (var i = 0; i < l; i++) {
    
    
    var stride = products[l - i - 1];

    
    var index = (index1d / stride) | 0;
    ARRAY_PUSH(result, index);

    
    index1d -= (index * stride);
  }

  return result;
}

function StepIndices(shape, indices) {
  for (var i = shape.length - 1; i >= 0; i--) {
    var indexi = indices[i] + 1;
    if (indexi < shape[i]) {
      indices[i] = indexi;
      return;
    }
    indices[i] = 0;
  }
}








function ParallelArrayConstructEmpty() {
  this.buffer = [];
  this.offset = 0;
  this.shape = [0];
  this.get = ParallelArrayGet1;
}





function ParallelArrayConstructFromArray(buffer) {
  var buffer = ToObject(buffer);
  var length = buffer.length >>> 0;
  if (length !== buffer.length)
    ThrowError(JSMSG_PAR_ARRAY_BAD_ARG, "");

  var buffer1 = [];
  for (var i = 0; i < length; i++)
    ARRAY_PUSH(buffer1, buffer[i]);

  this.buffer = buffer1;
  this.offset = 0;
  this.shape = [length];
  this.get = ParallelArrayGet1;
}








function ParallelArrayConstructFromFunction(shape, func) {
  return ParallelArrayConstructFromComprehension(this, shape, func, undefined);
}





function ParallelArrayConstructFromFunctionMode(shape, func, mode) {
  return ParallelArrayConstructFromComprehension(this, shape, func, mode);
}












function ParallelArrayConstructFromComprehension(self, shape, func, mode) {
  

  if (typeof shape === "number") {
    var length = shape >>> 0;
    if (length !== shape)
      ThrowError(JSMSG_PAR_ARRAY_BAD_ARG, "");
    ParallelArrayBuild(self, [length], func, mode);
  } else if (!shape || typeof shape.length !== "number") {
    ThrowError(JSMSG_PAR_ARRAY_BAD_ARG, "");
  } else {
    var shape1 = [];
    for (var i = 0, l = shape.length; i < l; i++) {
      var s0 = shape[i];
      var s1 = s0 >>> 0;
      if (s1 !== s0)
        ThrowError(JSMSG_PAR_ARRAY_BAD_ARG, "");
      ARRAY_PUSH(shape1, s1);
    }
    ParallelArrayBuild(self, shape1, func, mode);
  }
}







function ParallelArrayView(shape, buffer, offset) {
  this.shape = shape;
  this.buffer = buffer;
  this.offset = offset;

  switch (shape.length) {
    case 1: this.get = ParallelArrayGet1; break;
    case 2: this.get = ParallelArrayGet2; break;
    case 3: this.get = ParallelArrayGet3; break;
    default: this.get = ParallelArrayGetN; break;
  }

  
  
  
  return this;
}







function ParallelArrayBuild(self, shape, func, mode) {
  self.offset = 0;
  self.shape = shape;

  var length;
  var xDimension, yDimension, zDimension;
  var computefunc;

  switch (shape.length) {
  case 1:
    length = shape[0];
    self.get = ParallelArrayGet1;
    computefunc = fill1;
    break;
  case 2:
    xDimension = shape[0];
    yDimension = shape[1];
    length = xDimension * yDimension;
    self.get = ParallelArrayGet2;
    computefunc = fill2;
    break;
  case 3:
    xDimension = shape[0];
    yDimension = shape[1];
    zDimension = shape[2];
    length = xDimension * yDimension * zDimension;
    self.get = ParallelArrayGet3;
    computefunc = fill3;
    break;
  default:
    length = 1;
    for (var i = 0; i < shape.length; i++)
      length *= shape[i];
    self.get = ParallelArrayGetN;
    computefunc = fillN;
    break;
  }

  var buffer = self.buffer = NewDenseArray(length);

  parallel: for (;;) {
    
    
    
    
    
    
    if (ShouldForceSequential())
      break parallel;
    if (!TRY_PARALLEL(mode))
      break parallel;
    if (computefunc === fillN)
      break parallel;

    var chunks = ComputeNumChunks(length);
    var numSlices = ForkJoinSlices();
    var info = ComputeAllSliceBounds(chunks, numSlices);
    ForkJoin(constructSlice, ForkJoinMode(mode));
    return;
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  computefunc(0, length);
  return;

  function constructSlice(sliceId, numSlices, warmup) {
    var chunkPos = info[SLICE_POS(sliceId)];
    var chunkEnd = info[SLICE_END(sliceId)];

    if (warmup && chunkEnd > chunkPos)
      chunkEnd = chunkPos + 1;

    while (chunkPos < chunkEnd) {
      var indexStart = chunkPos << CHUNK_SHIFT;
      var indexEnd = std_Math_min(indexStart + CHUNK_SIZE, length);
      computefunc(indexStart, indexEnd);
      UnsafePutElements(info, SLICE_POS(sliceId), ++chunkPos);
    }

    return chunkEnd == info[SLICE_END(sliceId)];
  }

  function fill1(indexStart, indexEnd) {
    for (var i = indexStart; i < indexEnd; i++)
      UnsafePutElements(buffer, i, func(i));
  }

  function fill2(indexStart, indexEnd) {
    var x = (indexStart / yDimension) | 0;
    var y = indexStart - x * yDimension;
    for (var i = indexStart; i < indexEnd; i++) {
      UnsafePutElements(buffer, i, func(x, y));
      if (++y == yDimension) {
        y = 0;
        ++x;
      }
    }
  }

  function fill3(indexStart, indexEnd) {
    var x = (indexStart / (yDimension * zDimension)) | 0;
    var r = indexStart - x * yDimension * zDimension;
    var y = (r / zDimension) | 0;
    var z = r - y * zDimension;
    for (var i = indexStart; i < indexEnd; i++) {
      UnsafePutElements(buffer, i, func(x, y, z));
      if (++z == zDimension) {
        z = 0;
        if (++y == yDimension) {
          y = 0;
          ++x;
        }
      }
    }
  }

  function fillN(indexStart, indexEnd) {
    var indices = ComputeIndices(shape, indexStart);
    for (var i = indexStart; i < indexEnd; i++) {
      var result = callFunction(std_Function_apply, func, null, indices);
      UnsafePutElements(buffer, i, result);
      StepIndices(shape, indices);
    }
  }
}






function ParallelArrayMap(func, mode) {
  
  

  var self = this;
  var length = self.shape[0];
  var buffer = NewDenseArray(length);

  parallel: for (;;) { 
    if (ShouldForceSequential())
      break parallel;
    if (!TRY_PARALLEL(mode))
      break parallel;

    var chunks = ComputeNumChunks(length);
    var numSlices = ForkJoinSlices();
    var info = ComputeAllSliceBounds(chunks, numSlices);
    ForkJoin(mapSlice, ForkJoinMode(mode));
    return NewParallelArray(ParallelArrayView, [length], buffer, 0);
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  for (var i = 0; i < length; i++) {
    
    var v = func(self.get(i), i, self);
    UnsafePutElements(buffer, i, v);
  }
  return NewParallelArray(ParallelArrayView, [length], buffer, 0);

  function mapSlice(sliceId, numSlices, warmup) {
    var chunkPos = info[SLICE_POS(sliceId)];
    var chunkEnd = info[SLICE_END(sliceId)];

    if (warmup && chunkEnd > chunkPos + 1)
      chunkEnd = chunkPos + 1;

    while (chunkPos < chunkEnd) {
      var indexStart = chunkPos << CHUNK_SHIFT;
      var indexEnd = std_Math_min(indexStart + CHUNK_SIZE, length);

      for (var i = indexStart; i < indexEnd; i++)
        UnsafePutElements(buffer, i, func(self.get(i), i, self));

      UnsafePutElements(info, SLICE_POS(sliceId), ++chunkPos);
    }

    return chunkEnd == info[SLICE_END(sliceId)];
  }
}





function ParallelArrayReduce(func, mode) {
  
  

  var self = this;
  var length = self.shape[0];

  if (length === 0)
    ThrowError(JSMSG_PAR_ARRAY_REDUCE_EMPTY);

  parallel: for (;;) { 
    if (ShouldForceSequential())
      break parallel;
    if (!TRY_PARALLEL(mode))
      break parallel;

    var chunks = ComputeNumChunks(length);
    var numSlices = ForkJoinSlices();
    if (chunks < numSlices)
      break parallel;

    var info = ComputeAllSliceBounds(chunks, numSlices);
    var subreductions = NewDenseArray(numSlices);
    ForkJoin(reduceSlice, ForkJoinMode(mode));
    var accumulator = subreductions[0];
    for (var i = 1; i < numSlices; i++)
      accumulator = func(accumulator, subreductions[i]);
    return accumulator;
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  var accumulator = self.get(0);
  for (var i = 1; i < length; i++)
    accumulator = func(accumulator, self.get(i));
  return accumulator;

  function reduceSlice(sliceId, numSlices, warmup) {
    var chunkStart = info[SLICE_START(sliceId)];
    var chunkPos = info[SLICE_POS(sliceId)];
    var chunkEnd = info[SLICE_END(sliceId)];

    
    
    
    
    
    
    

    if (warmup && chunkEnd > chunkPos + 2)
      chunkEnd = chunkPos + 2;

    if (chunkStart === chunkPos) {
      var indexPos = chunkStart << CHUNK_SHIFT;
      var accumulator = reduceChunk(self.get(indexPos), indexPos + 1, indexPos + CHUNK_SIZE);

      UnsafePutElements(subreductions, sliceId, accumulator, 
                        info, SLICE_POS(sliceId), ++chunkPos);
    }

    var accumulator = subreductions[sliceId]; 

    while (chunkPos < chunkEnd) {
      var indexPos = chunkPos << CHUNK_SHIFT;
      accumulator = reduceChunk(accumulator, indexPos, indexPos + CHUNK_SIZE);
      UnsafePutElements(subreductions, sliceId, accumulator, info, SLICE_POS(sliceId), ++chunkPos);
    }

    return chunkEnd == info[SLICE_END(sliceId)];
  }

  function reduceChunk(accumulator, from, to) {
    to = std_Math_min(to, length);
    for (var i = from; i < to; i++)
      accumulator = func(accumulator, self.get(i));
    return accumulator;
  }
}







function ParallelArrayScan(func, mode) {
  
  

  var self = this;
  var length = self.shape[0];

  if (length === 0)
    ThrowError(JSMSG_PAR_ARRAY_REDUCE_EMPTY);

  var buffer = NewDenseArray(length);

  parallel: for (;;) { 
    if (ShouldForceSequential())
      break parallel;
    if (!TRY_PARALLEL(mode))
      break parallel;

    var chunks = ComputeNumChunks(length);
    var numSlices = ForkJoinSlices();
    if (chunks < numSlices)
      break parallel;
    var info = ComputeAllSliceBounds(chunks, numSlices);

    
    ForkJoin(phase1, ForkJoinMode(mode));

    
    var intermediates = [];
    var accumulator = buffer[finalElement(0)];
    ARRAY_PUSH(intermediates, accumulator);
    for (var i = 1; i < numSlices - 1; i++) {
      accumulator = func(accumulator, buffer[finalElement(i)]);
      ARRAY_PUSH(intermediates, accumulator);
    }

    
    
    for (var i = 0; i < numSlices; i++) {
      info[SLICE_POS(i)] = info[SLICE_START(i)] << CHUNK_SHIFT;
      info[SLICE_END(i)] = info[SLICE_END(i)] << CHUNK_SHIFT;
    }
    info[SLICE_END(numSlices - 1)] = std_Math_min(info[SLICE_END(numSlices - 1)], length);

    
    ForkJoin(phase2, ForkJoinMode(mode));
    return NewParallelArray(ParallelArrayView, [length], buffer, 0);
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  scan(self.get(0), 0, length);
  return NewParallelArray(ParallelArrayView, [length], buffer, 0);

  function scan(accumulator, start, end) {
    UnsafePutElements(buffer, start, accumulator);
    for (var i = start + 1; i < end; i++) {
      accumulator = func(accumulator, self.get(i));
      UnsafePutElements(buffer, i, accumulator);
    }
    return accumulator;
  }

  















  function phase1(sliceId, numSlices, warmup) {
    var chunkStart = info[SLICE_START(sliceId)];
    var chunkPos = info[SLICE_POS(sliceId)];
    var chunkEnd = info[SLICE_END(sliceId)];

    if (warmup && chunkEnd > chunkPos + 2)
      chunkEnd = chunkPos + 2;

    if (chunkPos == chunkStart) {
      
      
      var indexStart = chunkPos << CHUNK_SHIFT;
      var indexEnd = std_Math_min(indexStart + CHUNK_SIZE, length);
      scan(self.get(indexStart), indexStart, indexEnd);
      UnsafePutElements(info, SLICE_POS(sliceId), ++chunkPos);
    }

    while (chunkPos < chunkEnd) {
      
      
      
      
      
      var indexStart = chunkPos << CHUNK_SHIFT;
      var indexEnd = std_Math_min(indexStart + CHUNK_SIZE, length);
      var accumulator = func(buffer[indexStart - 1], self.get(indexStart));
      scan(accumulator, indexStart, indexEnd);
      UnsafePutElements(info, SLICE_POS(sliceId), ++chunkPos);
    }

    return chunkEnd == info[SLICE_END(sliceId)];
  }

  


  function finalElement(sliceId) {
    var chunkEnd = info[SLICE_END(sliceId)]; 
    var indexStart = std_Math_min(chunkEnd << CHUNK_SHIFT, length);
    return indexStart - 1;
  }

  








































  function phase2(sliceId, numSlices, warmup) {
    if (sliceId == 0)
      return true; 

    var indexPos = info[SLICE_POS(sliceId)];
    var indexEnd = info[SLICE_END(sliceId)];

    if (warmup)
      indexEnd = std_Math_min(indexEnd, indexPos + CHUNK_SIZE);

    var intermediate = intermediates[sliceId - 1];
    for (; indexPos < indexEnd; indexPos++) {
      UnsafePutElements(buffer, indexPos, func(intermediate, buffer[indexPos]),
                        info, SLICE_POS(sliceId), indexPos + 1);
    }

    return indexEnd == info[SLICE_END(sliceId)];
  }
}























function ParallelArrayScatter(targets, defaultValue, conflictFunc, length, mode) {
  
  
  

  var self = this;

  if (length === undefined)
    length = self.shape[0];

  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (targets.length >>> 0 !== targets.length)
    ThrowError(JSMSG_BAD_ARRAY_LENGTH, ".prototype.scatter");

  var targetsLength = std_Math_min(targets.length, self.length);

  if (length >>> 0 !== length)
    ThrowError(JSMSG_BAD_ARRAY_LENGTH, ".prototype.scatter");

  parallel: for (;;) { 
    if (ShouldForceSequential())
      break parallel;
    if (!TRY_PARALLEL(mode))
      break parallel;

    if (forceDivideScatterVector())
      return parDivideScatterVector();
    else if (forceDivideOutputRange())
      return parDivideOutputRange();
    else if (conflictFunc === undefined && targetsLength < length)
      return parDivideOutputRange();
    return parDivideScatterVector();
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  return seq();

  function forceDivideScatterVector() {
    return mode && mode.strategy && mode.strategy == "divide-scatter-vector";
  }

  function forceDivideOutputRange() {
    return mode && mode.strategy && mode.strategy == "divide-output-range";
  }

  function collide(elem1, elem2) {
    if (conflictFunc === undefined)
      ThrowError(JSMSG_PAR_ARRAY_SCATTER_CONFLICT);

    return conflictFunc(elem1, elem2);
  }


  function parDivideOutputRange() {
    var chunks = ComputeNumChunks(targetsLength);
    var numSlices = ForkJoinSlices();
    var checkpoints = NewDenseArray(numSlices);
    for (var i = 0; i < numSlices; i++)
      UnsafePutElements(checkpoints, i, 0);

    var buffer = NewDenseArray(length);
    var conflicts = NewDenseArray(length);

    for (var i = 0; i < length; i++) {
      UnsafePutElements(buffer, i, defaultValue);
      UnsafePutElements(conflicts, i, false);
    }

    ForkJoin(fill, ForkJoinMode(mode));
    return NewParallelArray(ParallelArrayView, [length], buffer, 0);

    function fill(sliceId, numSlices, warmup) {
      var indexPos = checkpoints[sliceId];
      var indexEnd = targetsLength;
      if (warmup)
        indexEnd = std_Math_min(indexEnd, indexPos + CHUNK_SIZE);

      
      var [outputStart, outputEnd] = ComputeSliceBounds(length, sliceId, numSlices);

      for (; indexPos < indexEnd; indexPos++) {
        var x = self.get(indexPos);
        var t = checkTarget(indexPos, targets[indexPos]);
        if (t < outputStart || t >= outputEnd)
          continue;
        if (conflicts[t])
          x = collide(x, buffer[t]);
        UnsafePutElements(buffer, t, x, conflicts, t, true, checkpoints, sliceId, indexPos + 1);
      }

      return indexEnd == targetsLength;
    }
  }

  function parDivideScatterVector() {
    
    
    
    
    
    var numSlices = ForkJoinSlices();
    var info = ComputeAllSliceBounds(targetsLength, numSlices);

    
    var localBuffers = NewDenseArray(numSlices);
    for (var i = 0; i < numSlices; i++)
      UnsafePutElements(localBuffers, i, NewDenseArray(length));
    var localConflicts = NewDenseArray(numSlices);
    for (var i = 0; i < numSlices; i++) {
      var conflicts_i = NewDenseArray(length);
      for (var j = 0; j < length; j++)
        UnsafePutElements(conflicts_i, j, false);
      UnsafePutElements(localConflicts, i, conflicts_i);
    }

    
    
    
    
    var outputBuffer = localBuffers[0];
    for (var i = 0; i < length; i++)
      UnsafePutElements(outputBuffer, i, defaultValue);

    ForkJoin(fill, ForkJoinMode(mode));
    mergeBuffers();
    return NewParallelArray(ParallelArrayView, [length], outputBuffer, 0);

    function fill(sliceId, numSlices, warmup) {
      var indexPos = info[SLICE_POS(sliceId)];
      var indexEnd = info[SLICE_END(sliceId)];
      if (warmup)
        indexEnd = std_Math_min(indexEnd, indexPos + CHUNK_SIZE);

      var localbuffer = localBuffers[sliceId];
      var conflicts = localConflicts[sliceId];
      while (indexPos < indexEnd) {
        var x = self.get(indexPos);
        var t = checkTarget(indexPos, targets[indexPos]);
        if (conflicts[t])
          x = collide(x, localbuffer[t]);
        UnsafePutElements(localbuffer, t, x, conflicts, t, true, 
                          info, SLICE_POS(sliceId), ++indexPos);
      }

      return indexEnd == info[SLICE_END(sliceId)];
    }

    




    function mergeBuffers() {
      var buffer = localBuffers[0];
      var conflicts = localConflicts[0];
      for (var i = 1; i < numSlices; i++) {
        var otherbuffer = localBuffers[i];
        var otherconflicts = localConflicts[i];
        for (var j = 0; j < length; j++) {
          if (otherconflicts[j]) {
            if (conflicts[j]) {
              buffer[j] = collide(otherbuffer[j], buffer[j]);
            } else {
              buffer[j] = otherbuffer[j];
              conflicts[j] = true;
            }
          }
        }
      }
    }
  }

  function seq() {
    var buffer = NewDenseArray(length);
    var conflicts = NewDenseArray(length);

    for (var i = 0; i < length; i++) {
      UnsafePutElements(buffer, i, defaultValue);
      UnsafePutElements(conflicts, i, false);
    }

    for (var i = 0; i < targetsLength; i++) {
      var x = self.get(i);
      var t = checkTarget(i, targets[i]);
      if (conflicts[t])
        x = collide(x, buffer[t]);

      UnsafePutElements(buffer, t, x, conflicts, t, true);
    }

    return NewParallelArray(ParallelArrayView, [length], buffer, 0);
  }

  function checkTarget(i, t) {
    if (TO_INT32(t) !== t)
      ThrowError(JSMSG_PAR_ARRAY_SCATTER_BAD_TARGET, i);

    if (t < 0 || t >= length)
      ThrowError(JSMSG_PAR_ARRAY_SCATTER_BOUNDS);

    
    return TO_INT32(t);
  }
}





function ParallelArrayFilter(func, mode) {
  
  

  var self = this;
  var length = self.shape[0];

  parallel: for (;;) { 
    if (ShouldForceSequential())
      break parallel;
    if (!TRY_PARALLEL(mode))
      break parallel;

    var chunks = ComputeNumChunks(length);
    var numSlices = ForkJoinSlices();
    if (chunks < numSlices * 2)
      break parallel;

    var info = ComputeAllSliceBounds(chunks, numSlices);

    
    
    
    
    
    
    
    
    var counts = NewDenseArray(numSlices);
    for (var i = 0; i < numSlices; i++)
      UnsafePutElements(counts, i, 0);
    var survivors = NewDenseArray(chunks);
    ForkJoin(findSurvivorsInSlice, ForkJoinMode(mode));

    
    var count = 0;
    for (var i = 0; i < numSlices; i++)
      count += counts[i];
    var buffer = NewDenseArray(count);
    if (count > 0)
      ForkJoin(copySurvivorsInSlice, ForkJoinMode(mode));

    return NewParallelArray(ParallelArrayView, [count], buffer, 0);
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  var buffer = [];
  for (var i = 0; i < length; i++) {
    var elem = self.get(i);
    if (func(elem, i, self))
      ARRAY_PUSH(buffer, elem);
  }
  return NewParallelArray(ParallelArrayView, [buffer.length], buffer, 0);

  





  function findSurvivorsInSlice(sliceId, numSlices, warmup) {
    var chunkPos = info[SLICE_POS(sliceId)];
    var chunkEnd = info[SLICE_END(sliceId)];

    if (warmup && chunkEnd > chunkPos)
      chunkEnd = chunkPos + 1;

    var count = counts[sliceId];
    while (chunkPos < chunkEnd) {
      var indexStart = chunkPos << CHUNK_SHIFT;
      var indexEnd = std_Math_min(indexStart + CHUNK_SIZE, length);
      var chunkBits = 0;

      for (var bit = 0; indexStart + bit < indexEnd; bit++) {
        var keep = !!func(self.get(indexStart + bit), indexStart + bit, self);
        chunkBits |= keep << bit;
        count += keep;
      }

      UnsafePutElements(survivors, chunkPos, chunkBits,
                        counts, sliceId, count,
                        info, SLICE_POS(sliceId), ++chunkPos);
    }

    return chunkEnd == info[SLICE_END(sliceId)];
  }

  function copySurvivorsInSlice(sliceId, numSlices, warmup) {
    
    
    
    

    
    var count = 0;
    if (sliceId > 0) { 
      for (var i = 0; i < sliceId; i++)
        count += counts[i];
    }

    
    var total = count + counts[sliceId];
    if (count == total)
      return true;

    
    
    
    
    
    var chunkStart = info[SLICE_START(sliceId)];
    var chunkEnd = info[SLICE_END(sliceId)];
    for (var chunk = chunkStart; chunk < chunkEnd; chunk++) {
      var chunkBits = survivors[chunk];
      if (!chunkBits)
        continue;

      var indexStart = chunk << CHUNK_SHIFT;
      for (var i = 0; i < CHUNK_SIZE; i++) {
        if (chunkBits & (1 << i)) {
          UnsafePutElements(buffer, count++, self.get(indexStart + i));
          if (count == total)
            break;
        }
      }
    }

    return true;
  }
}








function ParallelArrayPartition(amount) {
  if (amount >>> 0 !== amount)
    ThrowError(JSMSG_PAR_ARRAY_BAD_ARG, "");

  var length = this.shape[0];
  var partitions = (length / amount) | 0;

  if (partitions * amount !== length)
    ThrowError(JSMSG_PAR_ARRAY_BAD_PARTITION);

  var shape = [partitions, amount];
  for (var i = 1; i < this.shape.length; i++)
    ARRAY_PUSH(shape, this.shape[i]);
  return NewParallelArray(ParallelArrayView, shape, this.buffer, this.offset);
}





function ParallelArrayFlatten() {
  if (this.shape.length < 2)
    ThrowError(JSMSG_PAR_ARRAY_ALREADY_FLAT);

  var shape = [this.shape[0] * this.shape[1]];
  for (var i = 2; i < this.shape.length; i++)
    ARRAY_PUSH(shape, this.shape[i]);
  return NewParallelArray(ParallelArrayView, shape, this.buffer, this.offset);
}








function ParallelArrayGet1(i) {
  if (i === undefined)
    return undefined;
  return this.buffer[this.offset + i];
}




function ParallelArrayGet2(x, y) {
  var xDimension = this.shape[0];
  var yDimension = this.shape[1];
  if (x === undefined)
    return undefined;
  if (x >= xDimension)
    return undefined;
  if (y === undefined)
    return NewParallelArray(ParallelArrayView, [yDimension], this.buffer, this.offset + x * yDimension);
  if (y >= yDimension)
    return undefined;
  var offset = y + x * yDimension;
  return this.buffer[this.offset + offset];
}




function ParallelArrayGet3(x, y, z) {
  var xDimension = this.shape[0];
  var yDimension = this.shape[1];
  var zDimension = this.shape[2];
  if (x === undefined)
    return undefined;
  if (x >= xDimension)
    return undefined;
  if (y === undefined)
    return NewParallelArray(ParallelArrayView, [yDimension, zDimension],
                            this.buffer, this.offset + x * yDimension * zDimension);
  if (y >= yDimension)
    return undefined;
  if (z === undefined)
    return NewParallelArray(ParallelArrayView, [zDimension],
                            this.buffer, this.offset + y * zDimension + x * yDimension * zDimension);
  if (z >= zDimension)
    return undefined;
  var offset = z + y*zDimension + x * yDimension * zDimension;
  return this.buffer[this.offset + offset];
}




function ParallelArrayGetN(...coords) {
  if (coords.length == 0)
    return undefined;

  var products = ComputeProducts(this.shape);

  
  
  
  
  var offset = this.offset;
  var sDimensionality = this.shape.length;
  var cDimensionality = coords.length;
  for (var i = 0; i < cDimensionality; i++) {
    if (coords[i] >= this.shape[i])
      return undefined;
    offset += coords[i] * products[sDimensionality - i - 1];
  }

  if (cDimensionality < sDimensionality) {
    var shape = callFunction(std_Array_slice, this.shape, cDimensionality);
    return NewParallelArray(ParallelArrayView, shape, this.buffer, offset);
  }
  return this.buffer[offset];
}


function ParallelArrayLength() {
  return this.shape[0];
}

function ParallelArrayToString() {
  var l = this.length;
  if (l == 0)
    return "";

  var open, close;
  if (this.shape.length > 1) {
    open = "<";
    close = ">";
  } else {
    open = close = "";
  }

  var result = "";
  for (var i = 0; i < l - 1; i++) {
    result += open + String(this.get(i)) + close;
    result += ",";
  }
  result += open + String(this.get(l - 1)) + close;
  return result;
}





function AssertSequentialIsOK(mode) {
  if (mode && mode.mode && mode.mode !== "seq" && ParallelTestsShouldPass())
    ThrowError(JSMSG_WRONG_VALUE, "parallel execution", "sequential was forced");
}

function ForkJoinMode(mode) {
  
  if (!mode || !mode.mode) {
    return 0;
  } else if (mode.mode === "compile") {
    return 1;
  } else if (mode.mode === "par") {
    return 2;
  } else if (mode.mode === "recover") {
    return 3;
  } else if (mode.mode === "bailout") {
    return 4;
  } else {
    ThrowError(JSMSG_PAR_ARRAY_BAD_ARG, "");
  }
}








SetScriptHints(ParallelArrayConstructEmpty, { cloneAtCallsite: true });
SetScriptHints(ParallelArrayConstructFromArray, { cloneAtCallsite: true });
SetScriptHints(ParallelArrayConstructFromFunction, { cloneAtCallsite: true });
SetScriptHints(ParallelArrayConstructFromFunctionMode, { cloneAtCallsite: true });
SetScriptHints(ParallelArrayConstructFromComprehension, { cloneAtCallsite: true });
SetScriptHints(ParallelArrayView,       { cloneAtCallsite: true });
SetScriptHints(ParallelArrayBuild,      { cloneAtCallsite: true });
SetScriptHints(ParallelArrayMap,        { cloneAtCallsite: true });
SetScriptHints(ParallelArrayReduce,     { cloneAtCallsite: true });
SetScriptHints(ParallelArrayScan,       { cloneAtCallsite: true });
SetScriptHints(ParallelArrayScatter,    { cloneAtCallsite: true });
SetScriptHints(ParallelArrayFilter,     { cloneAtCallsite: true });







SetScriptHints(ParallelArrayGet1,       { cloneAtCallsite: true, inline: true });
SetScriptHints(ParallelArrayGet2,       { cloneAtCallsite: true, inline: true });
SetScriptHints(ParallelArrayGet3,       { cloneAtCallsite: true, inline: true });
