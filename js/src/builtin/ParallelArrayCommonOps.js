



































#define TRY_PARALLEL(MODE) \
  ((!MODE || MODE.mode === "par"))
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





function ComputeNumChunks(length) {
  var chunks = length >>> CHUNK_SHIFT;
  if (chunks << CHUNK_SHIFT === length)
    return chunks;
  return chunks + 1;
}







function ComputeSliceBounds(numItems, sliceIndex, numSlices) {
  var sliceWidth = TO_INT32(numItems / numSlices);
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





function PA_MAP_NAME(func, mode) {
  if (!IsCallable(func))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, func));

  var self = ToObject(this);
  var length = PA_LENGTH(self);
  var buffer = NewDenseArray(length);

  parallel: for (;;) {
    
    
    
    
    
    
    if (ShouldForceSequential())
      break parallel;
    if (!TRY_PARALLEL(mode))
      break parallel;

    var chunks = ComputeNumChunks(length);
    var numSlices = ForkJoinSlices();
    var info = ComputeAllSliceBounds(chunks, numSlices);
    ForkJoin(mapSlice, CheckParallel(mode));
    return PA_NEW(length, buffer, 0);
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  for (var i = 0; i < length; i++) {
    
    var v = func(PA_GET(self, i), i, self);
    UnsafeSetElement(buffer, i, v);
  }
  return PA_NEW(length, buffer, 0);

  function mapSlice(sliceId, numSlices, warmup) {
    var chunkPos = info[SLICE_POS(sliceId)];
    var chunkEnd = info[SLICE_END(sliceId)];

    if (warmup && chunkEnd > chunkPos + 1)
      chunkEnd = chunkPos + 1;

    while (chunkPos < chunkEnd) {
      var indexStart = chunkPos << CHUNK_SHIFT;
      var indexEnd = std_Math_min(indexStart + CHUNK_SIZE, length);

      for (var i = indexStart; i < indexEnd; i++)
        UnsafeSetElement(buffer, i, func(PA_GET(self, i), i, self));

      UnsafeSetElement(info, SLICE_POS(sliceId), ++chunkPos);
    }
  }
}





function PA_REDUCE_NAME(func, mode) {
  if (!IsCallable(func))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, func));

  var self = ToObject(this);
  var length = PA_LENGTH(self);

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
    ForkJoin(reduceSlice, CheckParallel(mode));
    var accumulator = subreductions[0];
    for (var i = 1; i < numSlices; i++)
      accumulator = func(accumulator, subreductions[i]);
    return accumulator;
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  var accumulator = PA_GET(self, 0);
  for (var i = 1; i < length; i++)
    accumulator = func(accumulator, PA_GET(self, i));
  return accumulator;

  function reduceSlice(sliceId, numSlices, warmup) {
    var chunkStart = info[SLICE_START(sliceId)];
    var chunkPos = info[SLICE_POS(sliceId)];
    var chunkEnd = info[SLICE_END(sliceId)];

    
    
    
    
    
    
    

    if (warmup && chunkEnd > chunkPos + 2)
      chunkEnd = chunkPos + 2;

    if (chunkStart === chunkPos) {
      var indexPos = chunkStart << CHUNK_SHIFT;
      var accumulator = reduceChunk(PA_GET(self, indexPos), indexPos + 1, indexPos + CHUNK_SIZE);

      UnsafeSetElement(subreductions, sliceId, accumulator, 
                       info, SLICE_POS(sliceId), ++chunkPos);
    }

    var accumulator = subreductions[sliceId]; 

    while (chunkPos < chunkEnd) {
      var indexPos = chunkPos << CHUNK_SHIFT;
      accumulator = reduceChunk(accumulator, indexPos, indexPos + CHUNK_SIZE);
      UnsafeSetElement(subreductions, sliceId, accumulator,
                       info, SLICE_POS(sliceId), ++chunkPos);
    }
  }

  function reduceChunk(accumulator, from, to) {
    to = std_Math_min(to, length);
    for (var i = from; i < to; i++)
      accumulator = func(accumulator, PA_GET(self, i));
    return accumulator;
  }
}






function PA_SCAN_NAME(func, mode) {
  if (!IsCallable(func))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, func));

  var self = ToObject(this);
  var length = PA_LENGTH(self);

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

    
    ForkJoin(phase1, CheckParallel(mode));

    
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

    
    ForkJoin(phase2, CheckParallel(mode));
    return PA_NEW(length, buffer, 0);
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  scan(PA_GET(self, 0), 0, length);
  return PA_NEW(length, buffer, 0);

  function scan(accumulator, start, end) {
    UnsafeSetElement(buffer, start, accumulator);
    for (var i = start + 1; i < end; i++) {
      accumulator = func(accumulator, PA_GET(self, i));
      UnsafeSetElement(buffer, i, accumulator);
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
      scan(PA_GET(self, indexStart), indexStart, indexEnd);
      UnsafeSetElement(info, SLICE_POS(sliceId), ++chunkPos);
    }

    while (chunkPos < chunkEnd) {
      
      
      
      
      
      var indexStart = chunkPos << CHUNK_SHIFT;
      var indexEnd = std_Math_min(indexStart + CHUNK_SIZE, length);
      var accumulator = func(buffer[indexStart - 1], PA_GET(self, indexStart));
      scan(accumulator, indexStart, indexEnd);
      UnsafeSetElement(info, SLICE_POS(sliceId), ++chunkPos);
    }
  }

  


  function finalElement(sliceId) {
    var chunkEnd = info[SLICE_END(sliceId)]; 
    var indexStart = std_Math_min(chunkEnd << CHUNK_SHIFT, length);
    return indexStart - 1;
  }

  








































  function phase2(sliceId, numSlices, warmup) {
    if (sliceId == 0)
      return; 

    var indexPos = info[SLICE_POS(sliceId)];
    var indexEnd = info[SLICE_END(sliceId)];

    if (warmup)
      indexEnd = std_Math_min(indexEnd, indexPos + CHUNK_SIZE);

    var intermediate = intermediates[sliceId - 1];
    for (; indexPos < indexEnd; indexPos++) {
      UnsafeSetElement(buffer, indexPos, func(intermediate, buffer[indexPos]),
                       info, SLICE_POS(sliceId), indexPos + 1);
    }
  }
}






















function PA_SCATTER_NAME(targets, defaultValue, conflictFunc, length, mode) {
  

  if (conflictFunc && !IsCallable(conflictFunc))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(2, conflictFunc));

  var self = ToObject(this);

  if (length === undefined)
    length = PA_LENGTH(self);

  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (!IS_UINT32(targets.length))
    ThrowError(JSMSG_BAD_ARRAY_LENGTH, ".prototype.scatter");

  var targetsLength = std_Math_min(targets.length, self.length);

  if (!IS_UINT32(length))
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
      UnsafeSetElement(checkpoints, i, 0);

    var buffer = NewDenseArray(length);
    var conflicts = NewDenseArray(length);

    for (var i = 0; i < length; i++) {
      UnsafeSetElement(buffer, i, defaultValue);
      UnsafeSetElement(conflicts, i, false);
    }

    ForkJoin(fill, CheckParallel(mode));
    return PA_NEW(length, buffer, 0);

    function fill(sliceId, numSlices, warmup) {
      var indexPos = checkpoints[sliceId];
      var indexEnd = targetsLength;
      if (warmup)
        indexEnd = std_Math_min(indexEnd, indexPos + CHUNK_SIZE);

      
      var [outputStart, outputEnd] = ComputeSliceBounds(length, sliceId, numSlices);

      for (; indexPos < indexEnd; indexPos++) {
        var x = PA_GET(self, indexPos);
        var t = checkTarget(indexPos, targets[indexPos]);
        if (t < outputStart || t >= outputEnd)
          continue;
        if (conflicts[t])
          x = collide(x, buffer[t]);
        UnsafeSetElement(buffer, t, x,
                         conflicts, t, true,
                         checkpoints, sliceId, indexPos + 1);
      }
    }
  }

  function parDivideScatterVector() {
    
    
    
    
    
    var numSlices = ForkJoinSlices();
    var info = ComputeAllSliceBounds(targetsLength, numSlices);

    
    var localBuffers = NewDenseArray(numSlices);
    for (var i = 0; i < numSlices; i++)
      UnsafeSetElement(localBuffers, i, NewDenseArray(length));
    var localConflicts = NewDenseArray(numSlices);
    for (var i = 0; i < numSlices; i++) {
      var conflicts_i = NewDenseArray(length);
      for (var j = 0; j < length; j++)
        UnsafeSetElement(conflicts_i, j, false);
      UnsafeSetElement(localConflicts, i, conflicts_i);
    }

    
    
    
    
    var outputBuffer = localBuffers[0];
    for (var i = 0; i < length; i++)
      UnsafeSetElement(outputBuffer, i, defaultValue);

    ForkJoin(fill, CheckParallel(mode));
    mergeBuffers();
    return PA_NEW(length, outputBuffer, 0);

    function fill(sliceId, numSlices, warmup) {
      var indexPos = info[SLICE_POS(sliceId)];
      var indexEnd = info[SLICE_END(sliceId)];
      if (warmup)
        indexEnd = std_Math_min(indexEnd, indexPos + CHUNK_SIZE);

      var localbuffer = localBuffers[sliceId];
      var conflicts = localConflicts[sliceId];
      while (indexPos < indexEnd) {
        var x = PA_GET(self, indexPos);
        var t = checkTarget(indexPos, targets[indexPos]);
        if (conflicts[t])
          x = collide(x, localbuffer[t]);
        UnsafeSetElement(localbuffer, t, x,
                         conflicts, t, true,
                         info, SLICE_POS(sliceId), ++indexPos);
      }
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
      UnsafeSetElement(buffer, i, defaultValue);
      UnsafeSetElement(conflicts, i, false);
    }

    for (var i = 0; i < targetsLength; i++) {
      var x = PA_GET(self, i);
      var t = checkTarget(i, targets[i]);
      if (conflicts[t])
        x = collide(x, buffer[t]);

      UnsafeSetElement(buffer, t, x,
                       conflicts, t, true);
    }

    return PA_NEW(length, buffer, 0);
  }

  function checkTarget(i, t) {
    if (TO_INT32(t) !== t)
      ThrowError(JSMSG_PAR_ARRAY_SCATTER_BAD_TARGET, i);

    if (t < 0 || t >= length)
      ThrowError(JSMSG_PAR_ARRAY_SCATTER_BOUNDS);

    
    return TO_INT32(t);
  }
}




function PA_FILTER_NAME(func, mode) {
  if (!IsCallable(func))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, func));

  var self = ToObject(this);
  var length = PA_LENGTH(self);

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
      UnsafeSetElement(counts, i, 0);
    var survivors = NewDenseArray(chunks);
    ForkJoin(findSurvivorsInSlice, CheckParallel(mode));

    
    var count = 0;
    for (var i = 0; i < numSlices; i++)
      count += counts[i];
    var buffer = NewDenseArray(count);
    if (count > 0)
      ForkJoin(copySurvivorsInSlice, CheckParallel(mode));

    return PA_NEW(count, buffer, 0);
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  var buffer = [];
  for (var i = 0; i < length; i++) {
    var elem = PA_GET(self, i);
    if (func(elem, i, self))
      ARRAY_PUSH(buffer, elem);
  }
  return PA_NEW(buffer.length, buffer, 0);

  





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

      UnsafeSetElement(survivors, chunkPos, chunkBits,
                       counts, sliceId, count,
                       info, SLICE_POS(sliceId), ++chunkPos);
    }
  }

  function copySurvivorsInSlice(sliceId, numSlices, warmup) {
    
    
    
    

    
    
    
    if (warmup && sliceId == 0 && numSlices != 1)
      sliceId = 1;

    
    var count = 0;
    if (sliceId > 0) { 
      for (var i = 0; i < sliceId; i++)
        count += counts[i];
    }

    
    var total = count + counts[sliceId];
    if (count == total)
      return;

    
    
    
    
    
    var chunkStart = info[SLICE_START(sliceId)];
    var chunkEnd = info[SLICE_END(sliceId)];
    for (var chunk = chunkStart; chunk < chunkEnd; chunk++) {
      var chunkBits = survivors[chunk];
      if (!chunkBits)
        continue;

      var indexStart = chunk << CHUNK_SHIFT;
      for (var i = 0; i < CHUNK_SIZE; i++) {
        if (chunkBits & (1 << i)) {
          UnsafeSetElement(buffer, count++, PA_GET(self, indexStart + i));
          if (count == total)
            break;
        }
      }
    }
  }
}






function AssertSequentialIsOK(mode) {
  if (mode && mode.mode && mode.mode !== "seq" && ParallelTestsShouldPass())
    ThrowError(JSMSG_WRONG_VALUE, "parallel execution", "sequential was forced");
}








function CheckParallel(mode) {
  if (!mode || !ParallelTestsShouldPass())
    return null;

  return function(result, bailouts, causes) {
    if (!("expect" in mode) || mode.expect === "any") {
      return; 
    } else if (mode.expect === "mixed" && result !== "disqualified") {
      return; 
    } else if (result === mode.expect) {
      return;
    }

    ThrowError(JSMSG_WRONG_VALUE, mode.expect,
               result+":"+bailouts+":"+causes);
  };
}








SetScriptHints(PA_MAP_NAME,        { cloneAtCallsite: true });
SetScriptHints(PA_REDUCE_NAME,     { cloneAtCallsite: true });
SetScriptHints(PA_SCAN_NAME,       { cloneAtCallsite: true });
SetScriptHints(PA_SCATTER_NAME,    { cloneAtCallsite: true });
SetScriptHints(PA_FILTER_NAME,     { cloneAtCallsite: true });
