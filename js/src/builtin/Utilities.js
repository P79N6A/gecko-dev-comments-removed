
























#include "SelfHostingDefines.h"







var std_Array_indexOf = ArrayIndexOf;
var std_String_substring = String_substring;

var std_WeakMap = WeakMap;

var std_StopIteration = StopIteration;






function List() {
    this.length = 0;
}

{
  let ListProto = std_Object_create(null);
  ListProto.indexOf = std_Array_indexOf;
  ListProto.join = std_Array_join;
  ListProto.push = std_Array_push;
  ListProto.slice = std_Array_slice;
  ListProto.sort = std_Array_sort;
  MakeConstructible(List, ListProto);
}






function Record() {
    return std_Object_create(null);
}
MakeConstructible(Record, {});






function HasProperty(o, p) {
    return p in o;
}



function ToBoolean(v) {
    return !!v;
}



function ToNumber(v) {
    return +v;
}



function CheckObjectCoercible(v) {
    if (v === undefined || v === null)
        ThrowError(JSMSG_CANT_CONVERT_TO, ToString(v), "object");
}


function ToLength(v) {
    v = ToInteger(v);

    if (v <= 0)
        return 0;

    
    return std_Math_min(v, 0x1fffffffffffff);
}


function SameValueZero(x, y) {
    return x === y || (x !== x && y !== y);
}


function GetMethod(O, P) {
    
    assert(IsPropertyKey(P), "Invalid property key");

    
    var func = ToObject(O)[P];

    
    if (func === undefined || func === null)
        return undefined;

    
    if (!IsCallable(func))
        ThrowError(JSMSG_NOT_FUNCTION, typeof func);

    
    return func;
}


function IsPropertyKey(argument) {
    var type = typeof argument;
    return type === "string" || type === "symbol";
}


function GetIterator(obj, method) {
    
    if (arguments.length === 1)
        method = GetMethod(obj, std_iterator);

    
    var iterator = callFunction(method, obj);

    
    if (!IsObject(iterator))
        ThrowError(JSMSG_NOT_ITERABLE, ToString(iterator));

    
    return iterator;
}

function SpeciesConstructor(obj, defaultConstructor) {
    var C = obj.constructor;
    if (C === undefined) {
        return defaultConstructor;
    }
    if (!IsConstructor(C)) {
        ThrowError(JSMSG_NOT_CONSTRUCTOR, DecompileArg(1, C));
    }
    return C;
}
