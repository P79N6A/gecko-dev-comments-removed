


























var std_isFinite = isFinite;
var std_isNaN = isNaN;
var std_Array_indexOf = ArrayIndexOf;
var std_Array_join = Array.prototype.join;
var std_Array_push = Array.prototype.push;
var std_Array_shift = Array.prototype.shift;
var std_Array_slice = Array.prototype.slice;
var std_Array_sort = Array.prototype.sort;
var std_Boolean_toString = Boolean.prototype.toString;
var Std_Date = Date;
var std_Date_now = Date.now;
var std_Function_bind = Function.prototype.bind;
var std_Function_apply = Function.prototype.apply;
var std_Math_floor = Math.floor;
var std_Math_max = Math.max;
var std_Math_min = Math.min;
var std_Object_create = Object.create;
var std_Object_defineProperty = Object.defineProperty;
var std_Object_getOwnPropertyNames = Object.getOwnPropertyNames;
var std_Object_hasOwnProperty = Object.prototype.hasOwnProperty;
var std_RegExp_test = RegExp.prototype.test;
var Std_String = String;
var std_String_indexOf = String.prototype.indexOf;
var std_String_lastIndexOf = String.prototype.lastIndexOf;
var std_String_match = String.prototype.match;
var std_String_replace = String.prototype.replace;
var std_String_split = String.prototype.split;
var std_String_startsWith = String.prototype.startsWith;
var std_String_substring = String.prototype.substring;
var std_String_toLowerCase = String.prototype.toLowerCase;
var std_String_toUpperCase = String.prototype.toUpperCase;
var std_WeakMap_get = WeakMap.prototype.get;
var std_WeakMap_has = WeakMap.prototype.has;
var std_WeakMap_set = WeakMap.prototype.set;






function List() {
    if (IS_UNDEFINED(List.prototype)) {
        var proto = std_Object_create(null);
        proto.indexOf = std_Array_indexOf;
        proto.join = std_Array_join;
        proto.push = std_Array_push;
        proto.slice = std_Array_slice;
        proto.sort = std_Array_sort;
        List.prototype = proto;
    }
}
MakeConstructible(List);






function Record() {
    return std_Object_create(null);
}
MakeConstructible(Record);






function HasProperty(o, p) {
    return p in o;
}



function ToBoolean(v) {
    return !!v;
}



function ToNumber(v) {
    return +v;
}



function ToString(v) {
    assert(arguments.length > 0, "__toString");
    return Std_String(v);
}






function IsObject(v) {
    
    
    
    
    return (typeof v === "object" && v !== null) ||
           (typeof v === "undefined" && v !== undefined);
}





function assert(b, info) {
    if (!b)
        AssertionFailed(info);
}

