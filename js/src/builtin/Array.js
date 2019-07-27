



 
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
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.indexOf');
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
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.lastIndexOf');
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
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.every');
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
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
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.every');
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return callFunction(ArrayEvery, list, callbackfn, T);
}


function ArraySome(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.some');
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
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
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.some');
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return callFunction(ArraySome, list, callbackfn, T);
}


function ArrayForEach(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.forEach');
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
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
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.map');
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
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
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.map');
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    return callFunction(ArrayMap, list, callbackfn, T);
}

function ArrayStaticForEach(list, callbackfn) {
    if (arguments.length < 2)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.forEach');
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    var T = arguments.length > 2 ? arguments[2] : void 0;
    callFunction(ArrayForEach, list, callbackfn, T);
}


function ArrayReduce(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.reduce');
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var k = 0;

    
    var accumulator;
    if (arguments.length > 1) {
        accumulator = arguments[1];
    } else {
        
        if (len === 0)
            ThrowTypeError(JSMSG_EMPTY_ARRAY_REDUCE);
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
              ThrowTypeError(JSMSG_EMPTY_ARRAY_REDUCE);
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
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.reduce');
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    if (arguments.length > 2)
        return callFunction(ArrayReduce, list, callbackfn, arguments[2]);
    else
        return callFunction(ArrayReduce, list, callbackfn);
}


function ArrayReduceRight(callbackfn) {
    
    var O = ToObject(this);

    
    var len = TO_UINT32(O.length);

    
    if (arguments.length === 0)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.reduce');
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var k = len - 1;

    
    var accumulator;
    if (arguments.length > 1) {
        accumulator = arguments[1];
    } else {
        
        if (len === 0)
            ThrowTypeError(JSMSG_EMPTY_ARRAY_REDUCE);
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
                ThrowTypeError(JSMSG_EMPTY_ARRAY_REDUCE);
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
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.reduceRight');
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(1, callbackfn));
    if (arguments.length > 2)
        return callFunction(ArrayReduceRight, list, callbackfn, arguments[2]);
    else
        return callFunction(ArrayReduceRight, list, callbackfn);
}


function ArrayFind(predicate) {
    
    var O = ToObject(this);

    
    var len = ToInteger(O.length);

    
    if (arguments.length === 0)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.find');
    if (!IsCallable(predicate))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, predicate));

    
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
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'Array.prototype.find');
    if (!IsCallable(predicate))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, predicate));

    
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



function ArrayIncludes(searchElement, fromIndex = 0) {
    
    var O = ToObject(this);

    
    var len = ToLength(O.length);

    
    if (len === 0)
        return false;

    
    var n = ToInteger(fromIndex);

    
    var k;
    if (n >= 0) {
        k = n;
    }
    
    else {
        
        k = len + n;
        
        if (k < 0)
            k = 0;
    }

    
    while (k < len) {
        
        if (SameValueZero(searchElement, O[k]))
            return true;

        
        k++;
    }

    
    return false;
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
    if (!IsObject(this) || !IsArrayIterator(this)) {
        return callFunction(CallArrayIteratorMethodIfWrapped, this,
                            "ArrayIteratorNext");
    }

    var a = UnsafeGetObjectFromReservedSlot(this, ARRAY_ITERATOR_SLOT_ITERATED_OBJECT);
    
    var index = UnsafeGetReservedSlot(this, ARRAY_ITERATOR_SLOT_NEXT_INDEX);
    var itemKind = UnsafeGetInt32FromReservedSlot(this, ARRAY_ITERATOR_SLOT_ITEM_KIND);
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


function ArrayFrom(items, mapfn=undefined, thisArg=undefined) {
    
    var C = this;

    
    var mapping = mapfn !== undefined;
    if (mapping && !IsCallable(mapfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(1, mapfn));
    var T = thisArg;

    
    var attrs = ATTR_CONFIGURABLE | ATTR_ENUMERABLE | ATTR_WRITABLE;

    
    var usingIterator = GetMethod(items, std_iterator);

    
    if (usingIterator !== undefined) {
        
        var A = IsConstructor(C) ? new C() : [];

        
        var iterator = GetIterator(items, usingIterator);

        
        var k = 0;

        
        
        
        while (true) {
            
            var next = iterator.next();
            if (!IsObject(next))
                ThrowTypeError(JSMSG_NEXT_RETURNED_PRIMITIVE);

            
            if (next.done) {
                A.length = k;
                return A;
            }

            
            var nextValue = next.value;

            
            var mappedValue = mapping ? callFunction(mapfn, thisArg, nextValue, k) : nextValue;

            
            _DefineDataProperty(A, k++, mappedValue, attrs);
        }
    }

    
    assert(usingIterator === undefined, "`items` can't be an Iterable after step 6.g.iv");

    
    var arrayLike = ToObject(items);

    
    var len = ToLength(arrayLike.length);

    
    var A = IsConstructor(C) ? new C(len) : NewDenseArray(len);

    
    for (var k = 0; k < len; k++) {
        
        var kValue = items[k];

        
        var mappedValue = mapping ? callFunction(mapfn, thisArg, kValue, k) : kValue;

        
        _DefineDataProperty(A, k, mappedValue, attrs);
    }

    
    A.length = len;

    
    return A;
}
