




function WeakSet_add(value) {
    
    var S = this;
    if (!IsObject(S) || !IsWeakSet(S))
        ThrowTypeError(JSMSG_INCOMPATIBLE_PROTO, "WeakSet", "add", typeof S);

    
    let entries = UnsafeGetReservedSlot(this, WEAKSET_MAP_SLOT);
    if (!entries)
        ThrowTypeError(JSMSG_INCOMPATIBLE_PROTO, "WeakSet", "add", typeof S);

    
    if (!IsObject(value))
        ThrowTypeError(JSMSG_NOT_NONNULL_OBJECT, DecompileArg(0, value));

    
    callFunction(std_WeakMap_set, entries, value, true);

    
    return S;
}


function WeakSet_clear() {
    
    var S = this;
    if (!IsObject(S) || !IsWeakSet(S))
        ThrowTypeError(JSMSG_INCOMPATIBLE_PROTO, "WeakSet", "clear", typeof S);

    
    let entries = UnsafeGetReservedSlot(this, WEAKSET_MAP_SLOT);
    if (!entries)
        ThrowTypeError(JSMSG_INCOMPATIBLE_PROTO, "WeakSet", "clear", typeof S);

    
    callFunction(std_WeakMap_clear, entries);

    
    return undefined;
}


function WeakSet_delete(value) {
    
    var S = this;
    if (!IsObject(S) || !IsWeakSet(S))
        ThrowTypeError(JSMSG_INCOMPATIBLE_PROTO, "WeakSet", "delete", typeof S);

    
    let entries = UnsafeGetReservedSlot(this, WEAKSET_MAP_SLOT);
    if (!entries)
        ThrowTypeError(JSMSG_INCOMPATIBLE_PROTO, "WeakSet", "delete", typeof S);

    
    if (!IsObject(value))
        return false;

    
    return callFunction(std_WeakMap_delete, entries, value);
}


function WeakSet_has(value) {
    
    var S = this;
    if (!IsObject(S) || !IsWeakSet(S))
        ThrowTypeError(JSMSG_INCOMPATIBLE_PROTO, "WeakSet", "has", typeof S);

    
    let entries = UnsafeGetReservedSlot(this, WEAKSET_MAP_SLOT);
    if (!entries)
        ThrowTypeError(JSMSG_INCOMPATIBLE_PROTO, "WeakSet", "has", typeof S);

    
    if (!IsObject(value))
        return false;

    
    return callFunction(std_WeakMap_has, entries, value);
}
