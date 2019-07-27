




function TypedArrayCopyWithin(target, start, end = undefined) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, target, start, end,
                            "TypedArrayCopyWithin");
    }

    

    
    var obj = this;

    
    var len = TypedArrayLength(obj);

    assert(0 <= len && len <= 0x7FFFFFFF,
           "assumed by some of the math below, see also the other assertions");

    
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

    assert(0 <= to && to <= 0x7FFFFFFF,
           "typed array |to| index assumed int32_t");
    assert(0 <= from && from <= 0x7FFFFFFF,
           "typed array |from| index assumed int32_t");

    
    
    
    
    
    
    assert(-0x7FFFFFFF - 1 <= count && count <= 0x7FFFFFFF,
           "typed array element count assumed int32_t");

    
    
    
    
    
    
    
    
    
    if (count > 0)
        MoveTypedArrayElements(obj, to | 0, from | 0, count | 0);

    
    return obj;
}


function TypedArrayEntries() {
    
    var O = this;

    
    if (!IsObject(O) || !IsTypedArray(O)) {
        return callFunction(CallTypedArrayMethodIfWrapped, O, "TypedArrayEntries");
    }

    

    
    return CreateArrayIterator(O, ITEM_KIND_KEY_AND_VALUE);
}


function TypedArrayEvery(callbackfn, thisArg = undefined) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, callbackfn, thisArg,
                            "TypedArrayEvery");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (arguments.length === 0)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, "%TypedArray%.prototype.every");
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var T = thisArg;

    
    
    for (var k = 0; k < len; k++) {
        
        var kValue = O[k];

        
        var testResult = callFunction(callbackfn, T, kValue, k, O);

        
        if (!testResult)
            return false;
    }

    
    return true;
}


function TypedArrayFill(value, start = 0, end = undefined) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, value, start, end,
                            "TypedArrayFill");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
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


function TypedArrayFilter(callbackfn, thisArg = undefined) {
    
    var O = this;

    
    
    if (!IsObject(O) || !IsTypedArray(O)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, callbackfn, thisArg,
                           "TypedArrayFilter");
    }

    
    var len = TypedArrayLength(O);

    
    if (arguments.length === 0)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, "%TypedArray%.prototype.filter");
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var T = thisArg;

    
    var defaultConstructor = _ConstructorForTypedArray(O);

    
    var C = SpeciesConstructor(O, defaultConstructor);

    
    var kept = new List();

    
    var captured = 0;

    
    for (var k = 0; k < len; k++) {
        
        var kValue = O[k];
        
        var selected = ToBoolean(callFunction(callbackfn, T, kValue, k, O));
        
        if (selected) {
            
            kept.push(kValue);
            
            captured++;
        }
    }

    
    var A = new C(captured);

    
    for (var n = 0; n < captured; n++) {
        
        A[n] = kept[n];
    }

    
    return A;
}


function TypedArrayFind(predicate, thisArg = undefined) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, predicate, thisArg,
                            "TypedArrayFind");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (arguments.length === 0)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, "%TypedArray%.prototype.find");
    if (!IsCallable(predicate))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, predicate));

    
    var T = thisArg;

    
    
    for (var k = 0; k < len; k++) {
        
        var kValue = O[k];
        
        if (callFunction(predicate, T, kValue, k, O))
            return kValue;
    }

    
    return undefined;
}


function TypedArrayFindIndex(predicate, thisArg = undefined) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, predicate, thisArg,
                            "TypedArrayFindIndex");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (arguments.length === 0)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, "%TypedArray%.prototype.findIndex");
    if (!IsCallable(predicate))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, predicate));

    
    var T = thisArg;

    
    
    for (var k = 0; k < len; k++) {
        
        if (callFunction(predicate, T, O[k], k, O))
            return k;
    }

    
    return -1;
}


function TypedArrayForEach(callbackfn, thisArg = undefined) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
	return callFunction(CallTypedArrayMethodIfWrapped, this, callbackfn, thisArg,
			    "TypedArrayForEach");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (arguments.length === 0)
	ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, 'TypedArray.prototype.forEach');
    if (!IsCallable(callbackfn))
	ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var T = thisArg;

    
    
    for (var k = 0; k < len; k++) {
	
	
	callFunction(callbackfn, T, O[k], k, O);
    }

    
    return undefined;
}


function TypedArrayIndexOf(searchElement, fromIndex = 0) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, searchElement, fromIndex,
                            "TypedArrayIndexOf");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (len === 0)
        return -1;

    
    var n = ToInteger(fromIndex);

    
    if (n >= len)
        return -1;

    var k;
    
    if (n >= 0) {
        k = n;
    }
    
    else {
        
        k = len + n;
        
        if (k < 0)
            k = 0;
    }

    
    
    for (; k < len; k++) {
        if (O[k] === searchElement)
            return k;
    }

    
    return -1;
}


function TypedArrayJoin(separator) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, separator, "TypedArrayJoin");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    var sep = separator === undefined ? "," : ToString(separator);

    
    if (len === 0)
        return "";

    
    var element0 = O[0];

    
    
    var R = ToString(element0);

    
    for (var k = 1; k < len; k++) {
        
        var S = R + sep;

        
        var element = O[k];

        
        
        var next = ToString(element);

        
        R = S + next;
    }

    
    return R;
}


function TypedArrayKeys() {
    
    var O = this;

    
    if (!IsObject(O) || !IsTypedArray(O)) {
        return callFunction(CallTypedArrayMethodIfWrapped, O, "TypedArrayKeys");
    }

    

    
    return CreateArrayIterator(O, ITEM_KIND_KEY);
}


function TypedArrayLastIndexOf(searchElement, fromIndex = undefined) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, searchElement, fromIndex,
                            "TypedArrayLastIndexOf");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (len === 0)
        return -1;

    
    var n = fromIndex === undefined ? len - 1 : ToInteger(fromIndex);

    
    var k = n >= 0 ? std_Math_min(n, len - 1) : len + n;

    
    
    for (; k >= 0; k--) {
        if (O[k] === searchElement)
            return k;
    }

    
    return -1;
}


function TypedArrayMap(callbackfn, thisArg = undefined) {
    
    var O = this;

    
    
    if (!IsObject(O) || !IsTypedArray(O)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, callbackfn, thisArg,
                            "TypedArrayMap");
    }

    
    var len = TypedArrayLength(O);

    
    if (arguments.length === 0)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, '%TypedArray%.prototype.map');
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var T = thisArg;

    
    var defaultConstructor = _ConstructorForTypedArray(O);

    
    var C = SpeciesConstructor(O, defaultConstructor);

    
    var A = new C(len);

    
    for (var k = 0; k < len; k++) {
        
        var mappedValue = callFunction(callbackfn, T, O[k], k, O);
        
        A[k] = mappedValue;
    }

    
    return A;
}


function TypedArrayReduce(callbackfn) {
    
    if (!IsObject(this) || !IsTypedArray(this))
        return callFunction(CallTypedArrayMethodIfWrapped, this, callbackfn, "TypedArrayReduce");

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (arguments.length === 0)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, "%TypedArray%.prototype.reduce");
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    if (len === 0 && arguments.length === 1)
        ThrowTypeError(JSMSG_EMPTY_ARRAY_REDUCE);

    
    var k = 0;

    
    
    var accumulator = arguments.length > 1 ? arguments[1] : O[k++];

    
    
    for (; k < len; k++) {
        accumulator = callFunction(callbackfn, undefined, accumulator, O[k], k, O);
    }

    
    return accumulator;
}


function TypedArrayReduceRight(callbackfn) {
    
    if (!IsObject(this) || !IsTypedArray(this))
        return callFunction(CallTypedArrayMethodIfWrapped, this, callbackfn, "TypedArrayReduceRight");

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (arguments.length === 0)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, "%TypedArray%.prototype.reduceRight");
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    if (len === 0 && arguments.length === 1)
        ThrowTypeError(JSMSG_EMPTY_ARRAY_REDUCE);

    
    var k = len - 1;

    
    
    var accumulator = arguments.length > 1 ? arguments[1] : O[k--];

    
    
    for (; k >= 0; k--) {
        accumulator = callFunction(callbackfn, undefined, accumulator, O[k], k, O);
    }

    
    return accumulator;
}


function TypedArrayReverse() {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, "TypedArrayReverse");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    var middle = std_Math_floor(len / 2);

    
    
    
    for (var lower = 0; lower !== middle; lower++) {
        
        var upper = len - lower - 1;

        
        var lowerValue = O[lower];

        
        var upperValue = O[upper];

        
        O[lower] = upperValue;
        O[upper] = lowerValue;
    }

    
    return O;
}


function TypedArraySlice(start, end) {

    
    var O = this;

    
    if (!IsObject(O) || !IsTypedArray(O)) {
        return callFunction(CallTypedArrayMethodIfWrapped, O, start, end, "TypedArraySlice");
    }

    
    var len = TypedArrayLength(O);

    
    var relativeStart = ToInteger(start);

    
    var k = relativeStart < 0
            ? std_Math_max(len + relativeStart, 0)
            : std_Math_min(relativeStart, len);

    
    var relativeEnd = end === undefined ? len : ToInteger(end);

    
    var final = relativeEnd < 0
                ? std_Math_max(len + relativeEnd, 0)
                : std_Math_min(relativeEnd, len);

    
    var count = std_Math_max(final - k, 0);

    
    var defaultConstructor = _ConstructorForTypedArray(O);

    
    var C = SpeciesConstructor(O, defaultConstructor);

    
    var A = new C(count);

    
    var n = 0;

    
    while (k < final) {
        
        A[n] = O[k];
        
        k++;
        
        n++;
    }

    
    return A;
}


function TypedArraySome(callbackfn, thisArg = undefined) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, callbackfn, thisArg,
                            "TypedArraySome");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
    if (arguments.length === 0)
        ThrowTypeError(JSMSG_MISSING_FUN_ARG, 0, "%TypedArray%.prototype.some");
    if (!IsCallable(callbackfn))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(0, callbackfn));

    
    var T = thisArg;

    
    
    for (var k = 0; k < len; k++) {
        
        var kValue = O[k];

        
        var testResult = callFunction(callbackfn, T, kValue, k, O);

        
        if (testResult)
            return true;
    }

    
    return false;
}


function TypedArraySubarray(begin, end) {
    
    var obj = this;

    
    
    if (!IsObject(obj) || !IsTypedArray(obj)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, begin, end,
                            "TypedArraySubarray");
    }

    
    var buffer = TypedArrayBuffer(obj);
    var srcLength = TypedArrayLength(obj);

    
    var relativeBegin = ToInteger(begin);
    var beginIndex = relativeBegin < 0 ? std_Math_max(srcLength + relativeBegin, 0)
                                       : std_Math_min(relativeBegin, srcLength);

    
    var relativeEnd = end === undefined ? srcLength : ToInteger(end);
    var endIndex = relativeEnd < 0 ? std_Math_max(srcLength + relativeEnd, 0)
                                   : std_Math_min(relativeEnd, srcLength);

    
    var newLength = std_Math_max(endIndex - beginIndex, 0);

    
    var elementShift = TypedArrayElementShift(obj);

    
    var srcByteOffset = TypedArrayByteOffset(obj);

    
    var beginByteOffset = srcByteOffset + (beginIndex << elementShift);

    
    var defaultConstructor = _ConstructorForTypedArray(obj);
    var constructor = SpeciesConstructor(obj, defaultConstructor);

    
    return new constructor(buffer, beginByteOffset, newLength);
}


function TypedArrayValues() {
    
    var O = this;

    
    if (!IsObject(O) || !IsTypedArray(O)) {
        return callFunction(CallTypedArrayMethodIfWrapped, O, "TypedArrayValues");
    }

    

    
    return CreateArrayIterator(O, ITEM_KIND_VALUE);
}



function TypedArrayIncludes(searchElement, fromIndex = 0) {
    
    if (!IsObject(this) || !IsTypedArray(this)) {
        return callFunction(CallTypedArrayMethodIfWrapped, this, searchElement,
                            fromIndex, "TypedArrayIncludes");
    }

    
    var O = this;

    
    var len = TypedArrayLength(O);

    
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


function TypedArrayStaticFrom(source, mapfn = undefined, thisArg = undefined) {
    
    var C = this;

    
    if (!IsConstructor(C))
        ThrowTypeError(JSMSG_NOT_CONSTRUCTOR, DecompileArg(1, C));

    
    var f = mapfn;

    
    if (f !== undefined && !IsCallable(f))
        ThrowTypeError(JSMSG_NOT_FUNCTION, DecompileArg(1, f));

    
    return TypedArrayFrom(C, undefined, source, f, thisArg);
}


function TypedArrayFrom(constructor, target, items, mapfn, thisArg) {
    
    var C = constructor;

    
    assert(C === undefined || target === undefined,
           "Neither of 'constructor' and 'target' is undefined");

    
    assert(IsConstructor(C) || C === undefined,
           "'constructor' is neither an constructor nor undefined");

    
    assert(target === undefined || IsTypedArray(target),
           "'target' is neither a typed array nor undefined");

    
    assert(IsCallable(mapfn) || mapfn === undefined,
           "'target' is neither a function nor undefined");

    
    var mapping = mapfn !== undefined;
    var T = thisArg;

    
    var usingIterator = GetMethod(items, std_iterator);

    
    if (usingIterator !== undefined) {
        
        var iterator = GetIterator(items, usingIterator);

        
        var values = new List();

        
        while (true) {
            
            var next = iterator.next();
            if (!IsObject(next))
                ThrowTypeError(JSMSG_NEXT_RETURNED_PRIMITIVE);

            
            if (next.done)
                break;
            values.push(next.value);
        }

        
        var len = values.length;

        
        
        
        var targetObj = new C(len);

        
        for (var k = 0; k < len; k++) {
            
            var kValue = values[k];

            
            var mappedValue = mapping ? callFunction(mapfn, T, kValue, k) : kValue;

            
            targetObj[k] = mappedValue;
        }

        
        
        
        

        
        return targetObj;
    }

    
    

    
    var arrayLike = ToObject(items);

    
    var len = ToLength(arrayLike.length);

    
    
    var targetObj = new C(len);

    
    for (var k = 0; k < len; k++) {
        
        var kValue = arrayLike[k];

        
        var mappedValue = mapping ? callFunction(mapfn, T, kValue, k) : kValue;

        
        targetObj[k] = mappedValue;
    }

    
    return targetObj;
}


function TypedArrayStaticOf() {
    
    var len = arguments.length;

    
    var items = arguments;

    
    var C = this;

    
    if (!IsConstructor(C))
        ThrowTypeError(JSMSG_NOT_CONSTRUCTOR, typeof C);

    var newObj = new C(len);

    
    for (var k = 0; k < len; k++)
        newObj[k] = items[k]

    
    return newObj;
}
