
























#include "SelfHostingDefines.h"







var std_Array_indexOf = ArrayIndexOf;
var std_String_substring = String_substring;

var std_WeakMap = WeakMap;

var std_StopIteration = StopIteration;






function List() {}
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
