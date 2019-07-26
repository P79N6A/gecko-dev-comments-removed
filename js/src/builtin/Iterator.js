



function IteratorIdentity() {
    return this;
}

var LegacyIteratorWrapperMap = new std_WeakMap();

function IteratorResult(value, done) {
    var result = std_Object_create(null);
    result.value = value;
    result.done = done;
    return result;
}

function LegacyIteratorNext(arg) {
    var iter = callFunction(std_WeakMap_get, LegacyIteratorWrapperMap, this);
    try {
        return IteratorResult(iter.next(arg), false);
    } catch (e) {
        if (e instanceof std_StopIteration)
            return IteratorResult(undefined, true);
        throw e;
    }
}

function LegacyIteratorThrow(exn) {
    var iter = callFunction(std_WeakMap_get, LegacyIteratorWrapperMap, this);
    try {
        return IteratorResult(iter.throw(exn), false);
    } catch (e) {
        if (e instanceof std_StopIteration)
            return IteratorResult(undefined, true);
        throw e;
    }
}

function LegacyIterator(iter) {
    callFunction(std_WeakMap_set, LegacyIteratorWrapperMap, this, iter);
}

function LegacyGeneratorIterator(iter) {
    callFunction(std_WeakMap_set, LegacyIteratorWrapperMap, this, iter);
}

var LegacyIteratorsInitialized = std_Object_create(null);

function InitLegacyIterators() {
    var props = std_Object_create(null);

    props.next = std_Object_create(null);
    props.next.value = LegacyIteratorNext;
    props.next.enumerable = false;
    props.next.configurable = true;
    props.next.writable = true;

    props[std_iterator] = std_Object_create(null);
    props[std_iterator].value = IteratorIdentity;
    props[std_iterator].enumerable = false;
    props[std_iterator].configurable = true;
    props[std_iterator].writable = true;

    var LegacyIteratorProto = std_Object_create(GetIteratorPrototype(), props);
    MakeConstructible(LegacyIterator, LegacyIteratorProto);

    props.throw = std_Object_create(null);
    props.throw.value = LegacyIteratorThrow;
    props.throw.enumerable = false;
    props.throw.configurable = true;
    props.throw.writable = true;

    var LegacyGeneratorIteratorProto = std_Object_create(GetIteratorPrototype(), props);
    MakeConstructible(LegacyGeneratorIterator, LegacyGeneratorIteratorProto);

    LegacyIteratorsInitialized.initialized = true;
}

function NewLegacyIterator(iter, wrapper) {
    if (!LegacyIteratorsInitialized.initialized)
        InitLegacyIterators();

    return new wrapper(iter);
}

function LegacyIteratorShim() {
    return NewLegacyIterator(ToObject(this), LegacyIterator);
}

function LegacyGeneratorIteratorShim() {
    return NewLegacyIterator(ToObject(this), LegacyGeneratorIterator);
}

function ArrayIterator() {
    return NewLegacyIterator(callFunction(std_Array_iterator, this), LegacyIterator);
}
