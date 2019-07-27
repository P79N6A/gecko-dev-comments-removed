



 
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
        
        var kValue = O[k];
        
        if (callFunction(predicate, T, kValue, k, O))
            return kValue;
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
        
        if (callFunction(predicate, T, O[k], k, O))
            return k;
    }

    
    return -1;
}


function ArrayCopyWithin(target, start, end = undefined) {
    
    var O = ToObject(this);

    
    var len = ToInteger(O.length);

    
    var relativeTarget = ToInteger(target);

    var to = relativeTarget < 0 ? std_Math_max(len + relativeTarget, 0)
                                : std_Math_min(relativeTarget, len);

    
    var relativeStart = ToInteger(start);

    var from = relativeStart < 0 ? std_Math_max(len + relativeStart, 0)
                                 : std_Math_min(relativeStart, len);

    
    var relativeEnd = end === undefined ? len
                                        : ToInteger(end);

    var final = relativeEnd < 0 ? std_Math_max(len + relativeEnd, 0)
                                : std_Math_min(relativeEnd, len);

    
    var count = std_Math_min(final - from, len - to);

    
    if (from < to && to < (from + count)) {
        from = from + count - 1;
        to = to + count - 1;
        
        while (count > 0) {
            if (from in O)
                O[to] = O[from];
            else
                delete O[to];

            from--;
            to--;
            count--;
        }
    } else {
        
        while (count > 0) {
            if (from in O)
                O[to] = O[from];
            else
                delete O[to];

            from++;
            to++;
            count--;
        }
    }

    
    return O;
}


function ArrayFill(value, start = 0, end = undefined) {
    
    var O = ToObject(this);

    
    
    var len = ToInteger(O.length);

    
    var relativeStart = ToInteger(start);

    
    var k = relativeStart < 0
            ? std_Math_max(len + relativeStart, 0)
            : std_Math_min(relativeStart, len);

    
    var relativeEnd = end === undefined ? len : ToInteger(end);

    
    var final = relativeEnd < 0
                ? std_Math_max(len + relativeEnd, 0)
                : std_Math_min(relativeEnd, len);

    
    for (; k < final; k++) {
        O[k] = value;
    }

    
    return O;
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
    var result = { value: undefined, done: false };

    
    if (index >= TO_UINT32(a.length)) {
        
        
        UnsafeSetReservedSlot(this, ARRAY_ITERATOR_SLOT_NEXT_INDEX, 0xffffffff);
        result.done = true;
        return result;
    }

    UnsafeSetReservedSlot(this, ARRAY_ITERATOR_SLOT_NEXT_INDEX, index + 1);

    if (itemKind === ITEM_KIND_VALUE) {
        result.value = a[index];
        return result;
    }

    if (itemKind === ITEM_KIND_KEY_AND_VALUE) {
        var pair = NewDenseArray(2);
        pair[0] = index;
        pair[1] = a[index];
        result.value = pair;
        return result;
    }

    assert(itemKind === ITEM_KIND_KEY, itemKind);
    result.value = index;
    return result;
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


function ArrayFrom(arrayLike, mapfn=undefined, thisArg=undefined) {
    
    var C = this;

    
    var items = ToObject(arrayLike);

    
    var mapping = (mapfn !== undefined);
    if (mapping && !IsCallable(mapfn))
        ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(1, mapfn));

    
    var attrs = ATTR_CONFIGURABLE | ATTR_ENUMERABLE | ATTR_WRITABLE;

    
    var usingIterator = items["@@iterator"];
    if (usingIterator !== undefined) {
        
        var A = IsConstructor(C) ? new C() : [];

        
        var iterator = callFunction(usingIterator, items);

        
        var k = 0;

        
        
        
        var next;
        while (true) {
            
            next = iterator.next();
            if (!IsObject(next))
                ThrowError(JSMSG_NEXT_RETURNED_PRIMITIVE);
            if (next.done)
                break;  
            var nextValue = next.value;

            
            var mappedValue = mapping ? callFunction(mapfn, thisArg, nextValue, k) : nextValue;

            
            _DefineDataProperty(A, k++, mappedValue, attrs);
        }
    } else {
        
        

        
        
        var len = ToInteger(items.length);

        
        var A = IsConstructor(C) ? new C(len) : NewDenseArray(len);

        
        for (var k = 0; k < len; k++) {
            
            var kValue = items[k];

            
            var mappedValue = mapping ? callFunction(mapfn, thisArg, kValue, k) : kValue;

            
            _DefineDataProperty(A, k, mappedValue, attrs);
        }
    }

    
    A.length = k;
    return A;
}

#ifdef ENABLE_PARALLEL_JS










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
    ForkJoin(mapThread, 0, slicesInfo.count, ForkJoinMode(mode), buffer);
    return buffer;
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  for (var i = 0; i < length; i++)
    UnsafePutElements(buffer, i, func(self[i], i, self));
  return buffer;

  function mapThread(workerId, sliceStart, sliceEnd) {
    var sliceShift = slicesInfo.shift;
    var sliceId;
    while (GET_SLICE(sliceStart, sliceEnd, sliceId)) {
      var indexStart = SLICE_START_INDEX(sliceShift, sliceId);
      var indexEnd = SLICE_END_INDEX(sliceShift, indexStart, length);
      for (var i = indexStart; i < indexEnd; i++)
        UnsafePutElements(buffer, i, func(self[i], i, self));
    }
    return sliceId;
  }
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
    var numSlices = slicesInfo.count;
    var subreductions = NewDenseArray(numSlices);

    ForkJoin(reduceThread, 0, numSlices, ForkJoinMode(mode), subreductions);

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

  function reduceThread(workerId, sliceStart, sliceEnd) {
    var sliceShift = slicesInfo.shift;
    var sliceId;
    while (GET_SLICE(sliceStart, sliceEnd, sliceId)) {
      var indexStart = SLICE_START_INDEX(sliceShift, sliceId);
      var indexEnd = SLICE_END_INDEX(sliceShift, indexStart, length);
      var accumulator = self[indexStart];
      for (var i = indexStart + 1; i < indexEnd; i++)
        accumulator = func(accumulator, self[i]);
      UnsafePutElements(subreductions, sliceId, accumulator);
    }
    return sliceId;
  }
}






function ArrayScanPar(func, mode) {
  if (!IsCallable(func))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, func));

  var self = ToObject(this);
  var length = self.length;

  if (length === 0)
    ThrowError(JSMSG_EMPTY_ARRAY_REDUCE);

  
  
  
  
  var buffer = NewDenseArray(length);
  var buffer2 = NewDenseArray(length);

  parallel: for (;;) { 
    if (ShouldForceSequential())
      break parallel;
    if (!TRY_PARALLEL(mode))
      break parallel;

    var slicesInfo = ComputeSlicesInfo(length);
    var numSlices = slicesInfo.count;

    
    ForkJoin(phase1, 0, numSlices, ForkJoinMode(mode), buffer);

    
    var intermediates = [];
    var accumulator = buffer[finalElement(0)];
    ARRAY_PUSH(intermediates, accumulator);
    for (var i = 1; i < numSlices - 1; i++) {
      accumulator = func(accumulator, buffer[finalElement(i)]);
      ARRAY_PUSH(intermediates, accumulator);
    }

    
    
    
    
    for ( var k=0, limit=finalElement(0) ; k <= limit ; k++ )
      buffer2[k] = buffer[k];
    ForkJoin(phase2, 1, numSlices, ForkJoinMode(mode), buffer2);
    return buffer2;
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

  















  function phase1(workerId, sliceStart, sliceEnd) {
    var sliceShift = slicesInfo.shift;
    var sliceId;
    while (GET_SLICE(sliceStart, sliceEnd, sliceId)) {
      var indexStart = SLICE_START_INDEX(sliceShift, sliceId);
      var indexEnd = SLICE_END_INDEX(sliceShift, indexStart, length);
      scan(self[indexStart], indexStart, indexEnd);
    }
    return sliceId;
  }

  


  function finalElement(sliceId) {
    var sliceShift = slicesInfo.shift;
    return SLICE_END_INDEX(sliceShift, SLICE_START_INDEX(sliceShift, sliceId), length) - 1;
  }

  
































  function phase2(workerId, sliceStart, sliceEnd) {
    var sliceShift = slicesInfo.shift;
    var sliceId;
    while (GET_SLICE(sliceStart, sliceEnd, sliceId)) {
      var indexPos = SLICE_START_INDEX(sliceShift, sliceId);
      var indexEnd = SLICE_END_INDEX(sliceShift, indexPos, length);
      var intermediate = intermediates[sliceId - 1];
      for (; indexPos < indexEnd; indexPos++)
        UnsafePutElements(buffer2, indexPos, func(intermediate, buffer[indexPos]));
    }
    return sliceId;
  }
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

  function collide(elem1, elem2) {
    if (conflictFunc === undefined)
      ThrowError(JSMSG_PAR_ARRAY_SCATTER_CONFLICT);

    return conflictFunc(elem1, elem2);
  }

  function checkTarget(i, t) {
    if (TO_INT32(t) !== t)
      ThrowError(JSMSG_PAR_ARRAY_SCATTER_BAD_TARGET, i);

    if (t < 0 || t >= length)
      ThrowError(JSMSG_PAR_ARRAY_SCATTER_BOUNDS);

    
    return TO_INT32(t);
  }
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

    
    
    
    
    
    var numSlices = slicesInfo.count;
    var counts = NewDenseArray(numSlices);
    for (var i = 0; i < numSlices; i++)
      UnsafePutElements(counts, i, 0);

    var survivors = new Uint8Array(length);
    ForkJoin(findSurvivorsThread, 0, numSlices, ForkJoinMode(mode), survivors);

    
    var count = 0;
    for (var i = 0; i < numSlices; i++)
      count += counts[i];
    var buffer = NewDenseArray(count);
    if (count > 0)
      ForkJoin(copySurvivorsThread, 0, numSlices, ForkJoinMode(mode), buffer);

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

  



  function findSurvivorsThread(workerId, sliceStart, sliceEnd) {
    var sliceShift = slicesInfo.shift;
    var sliceId;
    while (GET_SLICE(sliceStart, sliceEnd, sliceId)) {
      var count = 0;
      var indexStart = SLICE_START_INDEX(sliceShift, sliceId);
      var indexEnd = SLICE_END_INDEX(sliceShift, indexStart, length);
      for (var indexPos = indexStart; indexPos < indexEnd; indexPos++) {
        var keep = !!func(self[indexPos], indexPos, self);
        UnsafePutElements(survivors, indexPos, keep);
        count += keep;
      }
      UnsafePutElements(counts, sliceId, count);
    }
    return sliceId;
  }

  





  function copySurvivorsThread(workerId, sliceStart, sliceEnd) {
    var sliceShift = slicesInfo.shift;
    var sliceId;
    while (GET_SLICE(sliceStart, sliceEnd, sliceId)) {
      
      var total = 0;
      for (var i = 0; i < sliceId + 1; i++)
        total += counts[i];

      
      var count = total - counts[sliceId];
      if (count === total)
        continue;

      var indexStart = SLICE_START_INDEX(sliceShift, sliceId);
      var indexEnd = SLICE_END_INDEX(sliceShift, indexStart, length);
      for (var indexPos = indexStart; indexPos < indexEnd; indexPos++) {
        if (survivors[indexPos]) {
          UnsafePutElements(buffer, count++, self[indexPos]);
          if (count == total)
            break;
        }
      }
    }

    return sliceId;
  }
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
    ForkJoin(constructThread, 0, slicesInfo.count, ForkJoinMode(mode), buffer);
    return buffer;
  }

  
  ASSERT_SEQUENTIAL_IS_OK(mode);
  for (var i = 0; i < length; i++)
    UnsafePutElements(buffer, i, func(i));
  return buffer;

  function constructThread(workerId, sliceStart, sliceEnd) {
    var sliceShift = slicesInfo.shift;
    var sliceId;
    while (GET_SLICE(sliceStart, sliceEnd, sliceId)) {
      var indexStart = SLICE_START_INDEX(sliceShift, sliceId);
      var indexEnd = SLICE_END_INDEX(sliceShift, indexStart, length);
      for (var i = indexStart; i < indexEnd; i++)
        UnsafePutElements(buffer, i, func(i));
    }
    return sliceId;
  }
}








SetScriptHints(ArrayMapPar,         { cloneAtCallsite: true });
SetScriptHints(ArrayReducePar,      { cloneAtCallsite: true });
SetScriptHints(ArrayScanPar,        { cloneAtCallsite: true });
SetScriptHints(ArrayScatterPar,     { cloneAtCallsite: true });
SetScriptHints(ArrayFilterPar,      { cloneAtCallsite: true });
SetScriptHints(ArrayStaticBuildPar, { cloneAtCallsite: true });

#endif 
