



 
function ArrayIndexOf(searchElement) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (len === 0)
        return -1;

    
    var n = arguments.length > 1 ? ToInteger(arguments[1]) : 0;

    
    if (n >= len)
        return -1;

    var k;
    
    if (n >= 0)
        k = n;
    
    else {
        
        k = len + n;
        
        if (k < 0)
            k = 0;
    }

    
    if (IsPackedArray(O)) {
        for (; k < len; k++) {
            if (O[k] === searchElement)
                return k;
        }
    } else {
        for (; k < len; k++) {
            if (k in O && O[k] === searchElement)
                return k;
        }
    }

    
    return -1;
}

function ArrayStaticIndexOf(list, searchElement) {
    if (arguments.length < 1)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.indexOf');
    var fromIndex = arguments.length > 2 ? arguments[2] : 0;
    return callFunction(ArrayIndexOf, list, searchElement, fromIndex);
}


function ArrayLastIndexOf(searchElement) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (len === 0)
        return -1;

    
    var n = arguments.length > 1 ? ToInteger(arguments[1]) : len - 1;

    
    var k;
    if (n > len - 1)
        k = len - 1;
    else if (n < 0)
        k = len + n;
    else
        k = n;

    
    if (IsPackedArray(O)) {
        for (; k >= 0; k--) {
            if (O[k] === searchElement)
                return k;
        }
    } else {
        for (; k >= 0; k--) {
            if (k in O && O[k] === searchElement)
                return k;
        }
    }

    
    return -1;
}

function ArrayStaticLastIndexOf(list, searchElement) {
    if (arguments.length < 1)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.lastIndexOf');
    var fromIndex;
    if (arguments.length > 2) {
        fromIndex = arguments[2];
    } else {
        var O = ToObject(list);
        var len = TO_UINT32(O.length);
        fromIndex = len - 1;
    }
    return callFunction(ArrayLastIndexOf, list, searchElement, fromIndex);
}


function ArrayEvery(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.every');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var T = arguments.length > 1 ? arguments[1] : void 0;

    
    
    for (var k = 0; k < len; k++) {
        
        if (k in O) {
            
            if (!callFunction(callbackfn, T, O[k], k, O))
                return false;
        }
    }

    
    return true;
}

function ArrayStaticEvery(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.every');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return callFunction(ArrayEvery, list, callbackfn, T);
}


function ArraySome(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.some');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var T = arguments.length > 1 ? arguments[1] : void 0;

    
    
    for (var k = 0; k < len; k++) {
        
        if (k in O) {
            
            if (callFunction(callbackfn, T, O[k], k, O))
                return true;
        }
    }

    
    return false;
}

function ArrayStaticSome(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.some');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return callFunction(ArraySome, list, callbackfn, T);
}


function ArrayForEach(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.forEach');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var T = arguments.length > 1 ? arguments[1] : void 0;

    
    
    for (var k = 0; k < len; k++) {
        
        if (k in O) {
            
            callFunction(callbackfn, T, O[k], k, O);
        }
    }

    
    return void 0;
}


function ArrayMap(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.map');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var T = arguments.length > 1 ? arguments[1] : void 0;

    
    var A = NewDenseArray(len);

    
    
    for (var k = 0; k < len; k++) {
        
        if (k in O) {
            
            var mappedValue = callFunction(callbackfn, T, O[k], k, O);
            
            UnsafePutElements(A, k, mappedValue);
        }
    }

    
    return A;
}

function ArrayStaticMap(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.map');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return callFunction(ArrayMap, list, callbackfn, T);
}

function ArrayStaticForEach(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.forEach');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    callFunction(ArrayForEach, list, callbackfn, T);
}


function ArrayReduce(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.reduce');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var k = 0;

    
    var accumulator;
    if (arguments.length > 1) {
        accumulator = arguments[1];
    } else {
        
        if (len === 0)
            ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);
        if (IsPackedArray(O)) {
            accumulator = O[k++];
        } else {
            var kPresent = false;
            for (; k < len; k++) {
                if (k in O) {
                    accumulator = O[k];
                    kPresent = true;
                    k++;
                    break;
                }
            }
            if (!kPresent)
              ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);
        }
    }

    
    
    for (; k < len; k++) {
        
        if (k in O) {
            
            accumulator = callbackfn(accumulator, O[k], k, O);
        }
    }

    
    return accumulator;
}

function ArrayStaticReduce(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.reduce');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    if (arguments.length > 2)
        return callFunction(ArrayReduce, list, callbackfn, arguments[2]);
    else
        return callFunction(ArrayReduce, list, callbackfn);
}


function ArrayReduceRight(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.reduce');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var k = len - 1;

    
    var accumulator;
    if (arguments.length > 1) {
        accumulator = arguments[1];
    } else {
        
        if (len === 0)
            ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);
        if (IsPackedArray(O)) {
            accumulator = O[k--];
        } else {
            var kPresent = false;
            for (; k >= 0; k--) {
                if (k in O) {
                    accumulator = O[k];
                    kPresent = true;
                    k--;
                    break;
                }
            }
            if (!kPresent)
                ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);
        }
    }

    
    
    for (; k >= 0; k--) {
        
        if (k in O) {
            
            accumulator = callbackfn(accumulator, O[k], k, O);
        }
    }

    
    return accumulator;
}

function ArrayStaticReduceRight(list, callbackfn) {
    if (arguments.length < 2)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.reduceRight');
    if (!IsCallable(callbackfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    if (arguments.length > 2)
        return callFunction(ArrayReduceRight, list, callbackfn, arguments[2]);
    else
        return callFunction(ArrayReduceRight, list, callbackfn);
}


function ArrayFind(predicate) {
    
    var O = ToObject(this);

    
    var len = ToInteger(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.find');
    if (!IsCallable(predicate))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, predicate));

    
    var T = arguments.length > 1 ? arguments[1] : undefined;

    
    
    




    for (var k = 0; k < len; k++) {
        
        if (k in O) {
            
            var kValue = O[k];
            if (callFunction(predicate, T, kValue, k, O))
                return kValue;
        }
    }

    
    return undefined;
}


function ArrayFindIndex(predicate) {
    
    var O = ToObject(this);

    
    var len = ToInteger(O.length);

    
    if (arguments.length === 0)
        ThrowError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.find');
    if (!IsCallable(predicate))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, predicate));

    
    var T = arguments.length > 1 ? arguments[1] : undefined;

    
    
    




    for (var k = 0; k < len; k++) {
        
        if (k in O) {
            
            if (callFunction(predicate, T, O[k], k, O))
                return k;
        }
    }

    
    return -1;
}

#define ARRAY_ITERATOR_SLOT_ITERATED_OBJECT 0
#define ARRAY_ITERATOR_SLOT_NEXT_INDEX 1
#define ARRAY_ITERATOR_SLOT_ITEM_KIND 2

#define ITEM_KIND_VALUE 0
#define ITEM_KIND_KEY_AND_VALUE 1
#define ITEM_KIND_KEY 2


function CreateArrayIteratorAt(obj, kind, n) {
    var iteratedObject = ToObject(obj);
    var iterator = NewArrayIterator();
    UnsafeSetReservedSlot(iterator, ARRAY_ITERATOR_SLOT_ITERATED_OBJECT, iteratedObject);
    UnsafeSetReservedSlot(iterator, ARRAY_ITERATOR_SLOT_NEXT_INDEX, n);
    UnsafeSetReservedSlot(iterator, ARRAY_ITERATOR_SLOT_ITEM_KIND, kind);
    return iterator;
}
function CreateArrayIterator(obj, kind) {
    return CreateArrayIteratorAt(obj, kind, 0);
}

function ArrayIteratorIdentity() {
    return this;
}

function ArrayIteratorNext() {
    
    if (!IsObject(this) || !IsArrayIterator(this))
        ThrowError(JSMSG_INCOMPATIBLE_METHOD, "ArrayIterator", "next", ToString(this));

    var a = UnsafeGetReservedSlot(this, ARRAY_ITERATOR_SLOT_ITERATED_OBJECT);
    var index = UnsafeGetReservedSlot(this, ARRAY_ITERATOR_SLOT_NEXT_INDEX);
    var itemKind = UnsafeGetReservedSlot(this, ARRAY_ITERATOR_SLOT_ITEM_KIND);

    
    if (index >= TO_UINT32(a.length)) {
        
        
        UnsafeSetReservedSlot(this, ARRAY_ITERATOR_SLOT_NEXT_INDEX, 0xffffffff);
        return { value: undefined, done: true };
    }

    UnsafeSetReservedSlot(this, ARRAY_ITERATOR_SLOT_NEXT_INDEX, index + 1);

    if (itemKind === ITEM_KIND_VALUE)
        return { value: a[index], done: false };

    if (itemKind === ITEM_KIND_KEY_AND_VALUE) {
        var pair = NewDenseArray(2);
        pair[0] = index;
        pair[1] = a[index];
        return { value: pair, done : false };
    }

    assert(itemKind === ITEM_KIND_KEY, itemKind);
    return { value: index, done: false };
}

function ArrayValuesAt(n) {
    return CreateArrayIteratorAt(this, ITEM_KIND_VALUE, n);
}

function ArrayValues() {
    return CreateArrayIterator(this, ITEM_KIND_VALUE);
}

function ArrayEntries() {
    return CreateArrayIterator(this, ITEM_KIND_KEY_AND_VALUE);
}

function ArrayKeys() {
    return CreateArrayIterator(this, ITEM_KIND_KEY);
}

#ifdef ENABLE_PARALLEL_JS







#define TRY_PARALLEL(MODE) \
  ((!MODE || MODE.mode !== "seq"))
#define ASSERT_SEQUENTIAL_IS_OK(MODE) \
  do { if (MODE) AssertSequentialIsOK(MODE) } while(false)


#define ARRAY_PUSH(ARRAY, ELEMENT) \
  callFunction(std_Array_push, ARRAY, ELEMENT);
#define ARRAY_SLICE(ARRAY, ELEMENT) \
  callFunction(std_Array_slice, ARRAY, ELEMENT);





#ifndef DEBUG
#define ParallelSpew(args)
#endif

#define MAX_SLICE_SHIFT 6
#define MAX_SLICE_SIZE 64
#define MAX_SLICES_PER_WORKER 8




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

  return { shift: shift, statuses: new Uint8Array(count), lastSequentialId: 0 };
}





#define SLICE_START(info, id) \
    (id << info.shift)
#define SLICE_END(info, start, length) \
    std_Math_min(start + (1 << info.shift), length)
#define SLICE_COUNT(info) \
    info.statuses.length







#define GET_SLICE(info, id) \
    ((id = ForkJoinGetSlice(InParallelSection() ? -1 : NextSequentialSliceId(info, -1))) >= 0)

#define SLICE_STATUS_DONE 1




#define MARK_SLICE_DONE(info, id) \
    UnsafePutElements(info.statuses, id, SLICE_STATUS_DONE)




function SlicesInfoClearStatuses(info) {
  var statuses = info.statuses;
  var length = statuses.length;
  for (var i = 0; i < length; i++)
    UnsafePutElements(statuses, i, 0);
  info.lastSequentialId = 0;
}





function NextSequentialSliceId(info, doneMarker) {
  var statuses = info.statuses;
  var length = statuses.length;
  for (var i = info.lastSequentialId; i < length; i++) {
    if (statuses[i] === SLICE_STATUS_DONE)
      continue;
    info.lastSequentialId = i;
    return i;
  }
  return doneMarker == undefined ? length : doneMarker;
}




function ShrinkLeftmost(info) {
  return function () {
    return [NextSequentialSliceId(info), SLICE_COUNT(info)]
  };
}





function ArrayMapPar(func, mode) {
  if (!IsCallable(func))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, func));

  var self = ToObject(this);
  var length = self.length;
  var buffer = NewDenseArray(length);

  parallel: for (;;) {
    
    
    
    
    
    
    if (ShouldForceSequential())
      break parallel;
    if (!TRY_PARALLEL(mode))
      break parallel;

    var slicesInfo = ComputeSlicesInfo(length);
    ForkJoin(mapThread, ShrinkLeftmost(slicesInfo), ForkJoinMode(mode));
    return buffer;
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  for (var i = 0; i < length; i++)
    UnsafePutElements(buffer, i, func(self[i], i, self));
  return buffer;

  function mapThread(warmup) {
    var sliceId;
    while (GET_SLICE(slicesInfo, sliceId)) {
      var indexStart = SLICE_START(slicesInfo, sliceId);
      var indexEnd = SLICE_END(slicesInfo, indexStart, length);
      for (var i = indexStart; i < indexEnd; i++)
        UnsafePutElements(buffer, i, func(self[i], i, self));
      MARK_SLICE_DONE(slicesInfo, sliceId);
      if (warmup)
        return;
    }
  }

  return undefined;
}





function ArrayReducePar(func, mode) {
  if (!IsCallable(func))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, func));

  var self = ToObject(this);
  var length = self.length;

  if (length === 0)
    ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);

  parallel: for (;;) { 
    if (ShouldForceSequential())
      break parallel;
    if (!TRY_PARALLEL(mode))
      break parallel;

    var slicesInfo = ComputeSlicesInfo(length);
    var numSlices = SLICE_COUNT(slicesInfo);
    var subreductions = NewDenseArray(numSlices);

    ForkJoin(reduceThread, ShrinkLeftmost(slicesInfo), ForkJoinMode(mode));

    var accumulator = subreductions[0];
    for (var i = 1; i < numSlices; i++)
      accumulator = func(accumulator, subreductions[i]);
    return accumulator;
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  var accumulator = self[0];
  for (var i = 1; i < length; i++)
    accumulator = func(accumulator, self[i]);
  return accumulator;

  function reduceThread(warmup) {
    var sliceId;
    while (GET_SLICE(slicesInfo, sliceId)) {
      var indexStart = SLICE_START(slicesInfo, sliceId);
      var indexEnd = SLICE_END(slicesInfo, indexStart, length);
      var accumulator = self[indexStart];
      for (var i = indexStart + 1; i < indexEnd; i++)
        accumulator = func(accumulator, self[i]);
      UnsafePutElements(subreductions, sliceId, accumulator);
      MARK_SLICE_DONE(slicesInfo, sliceId);
      if (warmup)
        return;
    }
  }

  return undefined;
}






function ArrayScanPar(func, mode) {
  if (!IsCallable(func))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, func));

  var self = ToObject(this);
  var length = self.length;

  if (length === 0)
    ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);

  var buffer = NewDenseArray(length);

  parallel: for (;;) { 
    if (ShouldForceSequential())
      break parallel;
    if (!TRY_PARALLEL(mode))
      break parallel;

    var slicesInfo = ComputeSlicesInfo(length);
    var numSlices = SLICE_COUNT(slicesInfo);

    
    ForkJoin(phase1, ShrinkLeftmost(slicesInfo), ForkJoinMode(mode));

    
    var intermediates = [];
    var accumulator = buffer[finalElement(0)];
    ARRAY_PUSH(intermediates, accumulator);
    for (var i = 1; i < numSlices - 1; i++) {
      accumulator = func(accumulator, buffer[finalElement(i)]);
      ARRAY_PUSH(intermediates, accumulator);
    }

    
    SlicesInfoClearStatuses(slicesInfo);

    
    MARK_SLICE_DONE(slicesInfo, 0);

    
    ForkJoin(phase2, ShrinkLeftmost(slicesInfo), ForkJoinMode(mode));
    return buffer;
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  scan(self[0], 0, length);
  return buffer;

  function scan(accumulator, start, end) {
    UnsafePutElements(buffer, start, accumulator);
    for (var i = start + 1; i < end; i++) {
      accumulator = func(accumulator, self[i]);
      UnsafePutElements(buffer, i, accumulator);
    }
    return accumulator;
  }

  















  function phase1(warmup) {
    var sliceId;
    while (GET_SLICE(slicesInfo, sliceId)) {
      var indexStart = SLICE_START(slicesInfo, sliceId);
      var indexEnd = SLICE_END(slicesInfo, indexStart, length);
      scan(self[indexStart], indexStart, indexEnd);
      MARK_SLICE_DONE(slicesInfo, sliceId);
      if (warmup)
        return;
    }
  }

  


  function finalElement(sliceId) {
    return SLICE_END(slicesInfo, SLICE_START(slicesInfo, sliceId), length) - 1;
  }

  
































  function phase2(warmup) {
    var sliceId;
    while (GET_SLICE(slicesInfo, sliceId)) {
      var indexPos = SLICE_START(slicesInfo, sliceId);
      var indexEnd = SLICE_END(slicesInfo, indexPos, length);

      var intermediate = intermediates[sliceId - 1];
      for (; indexPos < indexEnd; indexPos++)
        UnsafePutElements(buffer, indexPos, func(intermediate, buffer[indexPos]));

      MARK_SLICE_DONE(slicesInfo, sliceId);
      if (warmup)
        return;
    }
  }

  return undefined;
}






















function ArrayScatterPar(targets, defaultValue, conflictFunc, length, mode) {
  if (conflictFunc && !IsCallable(conflictFunc))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(2, conflictFunc));

  var self = ToObject(this);

  if (length === undefined)
    length = self.length;

  var targetsLength = std_Math_min(targets.length, self.length);

  if (!IS_UINT32(targetsLength) || !IS_UINT32(length))
    ThrowError(JSMSG_BAD_ARRAY_LENGTH);

  

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  return seq();

  function seq() {
    var buffer = NewDenseArray(length);
    var conflicts = NewDenseArray(length);

    for (var i = 0; i < length; i++) {
      UnsafePutElements(buffer, i, defaultValue);
      UnsafePutElements(conflicts, i, false);
    }

    for (var i = 0; i < targetsLength; i++) {
      var x = self[i];
      var t = checkTarget(i, targets[i]);
      if (conflicts[t])
        x = collide(x, buffer[t]);

      UnsafePutElements(buffer, t, x, conflicts, t, true);
    }

    return buffer;
  }

  function checkTarget(i, t) {
    if (TO_INT32(t) !== t)
      ThrowError(JSMSG_PAR_ARRAY_SCATTER_BAD_TARGET, i);

    if (t < 0 || t >= length)
      ThrowError(JSMSG_PAR_ARRAY_SCATTER_BOUNDS);

    
    return TO_INT32(t);
  }

  return undefined;
}




function ArrayFilterPar(func, mode) {
  if (!IsCallable(func))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, func));

  var self = ToObject(this);
  var length = self.length;

  parallel: for (;;) { 
    if (ShouldForceSequential())
      break parallel;
    if (!TRY_PARALLEL(mode))
      break parallel;

    var slicesInfo = ComputeSlicesInfo(length);

    
    
    
    
    
    
    
    
    var numSlices = SLICE_COUNT(slicesInfo);
    var counts = NewDenseArray(numSlices);
    for (var i = 0; i < numSlices; i++)
      UnsafePutElements(counts, i, 0);
    var survivors = NewDenseArray(computeNum32BitChunks(length));
    ForkJoin(findSurvivorsThread, ShrinkLeftmost(slicesInfo), ForkJoinMode(mode));

    
    SlicesInfoClearStatuses(slicesInfo);

    
    var count = 0;
    for (var i = 0; i < numSlices; i++)
      count += counts[i];
    var buffer = NewDenseArray(count);
    if (count > 0)
      ForkJoin(copySurvivorsThread, ShrinkLeftmost(slicesInfo), ForkJoinMode(mode));

    return buffer;
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  var buffer = [];
  for (var i = 0; i < length; i++) {
    var elem = self[i];
    if (func(elem, i, self))
      ARRAY_PUSH(buffer, elem);
  }
  return buffer;

  


  function computeNum32BitChunks(length) {
    var chunks = length >>> 5;
    if (chunks << 5 === length)
      return chunks;
    return chunks + 1;
  }

  





  function findSurvivorsThread(warmup) {
    var sliceId;
    while (GET_SLICE(slicesInfo, sliceId)) {
      var count = 0;
      var indexStart = SLICE_START(slicesInfo, sliceId);
      var indexEnd = SLICE_END(slicesInfo, indexStart, length);
      var chunkStart = computeNum32BitChunks(indexStart);
      var chunkEnd = computeNum32BitChunks(indexEnd);
      for (var chunkPos = chunkStart; chunkPos < chunkEnd; chunkPos++, indexStart += 32) {
        var chunkBits = 0;
        for (var bit = 0, indexPos = indexStart; bit < 32 && indexPos < indexEnd; bit++, indexPos++) {
          var keep = !!func(self[indexPos], indexPos, self);
          chunkBits |= keep << bit;
          count += keep;
        }
        UnsafePutElements(survivors, chunkPos, chunkBits);
      }
      UnsafePutElements(counts, sliceId, count);

      MARK_SLICE_DONE(slicesInfo, sliceId);
      if (warmup)
        return;
    }
  }

  function copySurvivorsThread(warmup) {
    var sliceId;
    while (GET_SLICE(slicesInfo, sliceId)) {
      
      
      
      

      
      var total = 0;
      for (var i = 0; i < sliceId + 1; i++)
        total += counts[i];

      
      var count = total - counts[sliceId];
      if (count === total) {
        MARK_SLICE_DONE(slicesInfo, sliceId);
        continue;
      }

      
      
      
      
      
      var indexStart = SLICE_START(slicesInfo, sliceId);
      var indexEnd = SLICE_END(slicesInfo, indexStart, length);
      var chunkStart = computeNum32BitChunks(indexStart);
      var chunkEnd = computeNum32BitChunks(indexEnd);
      for (var chunkPos = chunkStart; chunkPos < chunkEnd; chunkPos++, indexStart += 32) {
        var chunkBits = survivors[chunkPos];
        if (!chunkBits)
          continue;

        for (var i = 0; i < 32; i++) {
          if (chunkBits & (1 << i)) {
            UnsafePutElements(buffer, count++, self[indexStart + i]);
            if (count === total)
              break;
          }
        }

        if (count == total)
          break;
      }

      MARK_SLICE_DONE(slicesInfo, sliceId);
      if (warmup)
        return;
    }
  }

  return undefined;
}









function ArrayStaticBuild(length, func) {
  if (!IS_UINT32(length))
    ThrowError(JSMSG_BAD_ARRAY_LENGTH);
  if (!IsCallable(func))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, func));

  var buffer = NewDenseArray(length);

  for (var i = 0; i < length; i++)
    UnsafePutElements(buffer, i, func(i));

  return buffer;
}

function ArrayStaticBuildPar(length, func, mode) {
  if (!IS_UINT32(length))
    ThrowError(JSMSG_BAD_ARRAY_LENGTH);
  if (!IsCallable(func))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, func));

  var buffer = NewDenseArray(length);

  parallel: for (;;) {
    if (ShouldForceSequential())
      break parallel;
    if (!TRY_PARALLEL(mode))
      break parallel;

    var slicesInfo = ComputeSlicesInfo(length);
    ForkJoin(constructThread, ShrinkLeftmost(slicesInfo), ForkJoinMode(mode));
    return buffer;
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  for (var i = 0; i < length; i++)
    UnsafePutElements(buffer, i, func(i));
  return buffer;

  function constructThread(warmup) {
    var sliceId;
    while (GET_SLICE(slicesInfo, sliceId)) {
      var indexStart = SLICE_START(slicesInfo, sliceId);
      var indexEnd = SLICE_END(slicesInfo, indexStart, length);
      for (var i = indexStart; i < indexEnd; i++)
        UnsafePutElements(buffer, i, func(i));
      MARK_SLICE_DONE(slicesInfo, sliceId);
      if (warmup)
        return;
    }
  }

  return undefined;
}








SetScriptHints(ArrayMapPar,         { cloneAtCallsite: true });
SetScriptHints(ArrayReducePar,      { cloneAtCallsite: true });
SetScriptHints(ArrayScanPar,        { cloneAtCallsite: true });
SetScriptHints(ArrayScatterPar,     { cloneAtCallsite: true });
SetScriptHints(ArrayFilterPar,      { cloneAtCallsite: true });
SetScriptHints(ArrayStaticBuildPar, { cloneAtCallsite: true });

#endif 
