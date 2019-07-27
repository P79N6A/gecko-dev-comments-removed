




function WeakSet_add(value) {
    
    var S = this;
    if (!IsObject(S) || !IsWeakSet(S))
        ThrowError(JSMSG_INCOMPATIBLE_PROTO, "WeakSet", "add", typeof S);

    
    if (!IsObject(value))
        ThrowError(JSMSG_NOT_NONNULL_OBJECT);

    
    let entries = UnsafeGetReservedSlot(this, WEAKSET_MAP_SLOT);
    
    callFunction(std_WeakMap_set, entries, value, true);

    
    return S;
}


function WeakSet_clear() {
    
    var S = this;
    if (!IsObject(S) || !IsWeakSet(S))
        ThrowError(JSMSG_INCOMPATIBLE_PROTO, "WeakSet", "clear", typeof S);

    
    let entries = UnsafeGetReservedSlot(this, WEAKSET_MAP_SLOT);
    callFunction(std_WeakMap_clear, entries);

    
    return undefined;
}


function WeakSet_delete(value) {
    
    var S = this;
    if (!IsObject(S) || !IsWeakSet(S))
        ThrowError(JSMSG_INCOMPATIBLE_PROTO, "WeakSet", "delete", typeof S);

    
    if (!IsObject(value))
        ThrowError(JSMSG_NOT_NONNULL_OBJECT);

    
    let entries = UnsafeGetReservedSlot(this, WEAKSET_MAP_SLOT);
    
    return callFunction(std_WeakMap_delete, entries, value);
}


function WeakSet_has(value) {
    
    var S = this;
    if (!IsObject(S) || !IsWeakSet(S))
        ThrowError(JSMSG_INCOMPATIBLE_PROTO, "WeakSet", "has", typeof S);

    
    if (!IsObject(value))
        ThrowError(JSMSG_NOT_NONNULL_OBJECT);

    
    let entries = UnsafeGetReservedSlot(this, WEAKSET_MAP_SLOT);
    
    return callFunction(std_WeakMap_has, entries, value);
}