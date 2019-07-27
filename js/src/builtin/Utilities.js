
























#include "SelfHostingDefines.h"


Object.defineProperty = null; 








var std_Array_indexOf = ArrayIndexOf;
var std_Array_iterator = Array.prototype.iterator;
var std_Array_join = Array.prototype.join;
var std_Array_push = Array.prototype.push;
var std_Array_pop = Array.prototype.pop;
var std_Array_shift = Array.prototype.shift;
var std_Array_slice = Array.prototype.slice;
var std_Array_sort = Array.prototype.sort;
var std_Array_unshift = Array.prototype.unshift;
var std_Boolean_toString = Boolean.prototype.toString;
var Std_Date = Date;
var std_Date_now = Date.now;
var std_Date_valueOf = Date.prototype.valueOf;
var std_Function_bind = Function.prototype.bind;
var std_Function_apply = Function.prototype.apply;
var std_Math_floor = Math.floor;
var std_Math_max = Math.max;
var std_Math_min = Math.min;
var std_Math_abs = Math.abs;
var std_Math_imul = Math.imul;
var std_Math_log2 = Math.log2;
var std_Number_valueOf = Number.prototype.valueOf;
var std_Number_POSITIVE_INFINITY = Number.POSITIVE_INFINITY;
var std_Object_create = Object.create;
var std_Object_getOwnPropertyNames = Object.getOwnPropertyNames;
var std_Object_hasOwnProperty = Object.prototype.hasOwnProperty;
var std_Object_getPrototypeOf = Object.getPrototypeOf;
var std_Object_getOwnPropertyDescriptor = Object.getOwnPropertyDescriptor;
var std_RegExp_test = RegExp.prototype.test;
var std_String_fromCharCode = String.fromCharCode;
var std_String_charCodeAt = String.prototype.charCodeAt;
var std_String_indexOf = String.prototype.indexOf;
var std_String_lastIndexOf = String.prototype.lastIndexOf;
var std_String_match = String.prototype.match;
var std_String_replace = String.prototype.replace;
var std_String_split = String.prototype.split;
var std_String_startsWith = String.prototype.startsWith;
var std_String_substring = String.prototype.substring;
var std_String_toLowerCase = String.prototype.toLowerCase;
var std_String_toUpperCase = String.prototype.toUpperCase;
var std_WeakMap = WeakMap;
var std_WeakMap_get = WeakMap.prototype.get;
var std_WeakMap_has = WeakMap.prototype.has;
var std_WeakMap_set = WeakMap.prototype.set;
var std_WeakMap_clear = WeakMap.prototype.clear;
var std_WeakMap_delete = WeakMap.prototype.delete;
var std_Map_has = Map.prototype.has;
var std_Set_has = Set.prototype.has;
var std_iterator = '@@iterator'; 
var std_StopIteration = StopIteration;
var std_Map_iterator = Map.prototype[std_iterator];
var std_Set_iterator = Set.prototype[std_iterator];
var std_Map_iterator_next = Object.getPrototypeOf(Map()[std_iterator]()).next;
var std_Set_iterator_next = Object.getPrototypeOf(Set()[std_iterator]()).next;







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

    
    return v < 0x1fffffffffffff ? v : 0x1fffffffffffff;
}



#ifdef ENABLE_PARALLEL_JS





function AssertSequentialIsOK(mode) {
  if (mode && mode.mode && mode.mode !== "seq" && ParallelTestsShouldPass())
    ThrowError(JSMSG_WRONG_VALUE, "parallel execution", "sequential was forced");
}

function ForkJoinMode(mode) {
  
  if (!mode || !mode.mode) {
    return 0;
  } else if (mode.mode === "compile") {
    return 1;
  } else if (mode.mode === "par") {
    return 2;
  } else if (mode.mode === "recover") {
    return 3;
  } else if (mode.mode === "bailout") {
    return 4;
  }
  ThrowError(JSMSG_PAR_ARRAY_BAD_ARG);
}

#endif
