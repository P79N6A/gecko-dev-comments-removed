




















var std_ArrayIndexOf = ArrayIndexOf;
var std_ArrayJoin = Array.prototype.join;
var std_ArrayPush = Array.prototype.push;
var std_ArraySlice = Array.prototype.slice;
var std_ArraySort = Array.prototype.sort;
var std_ObjectCreate = Object.create;
var std_String = String;






function List() {
    if (IS_UNDEFINED(List.prototype)) {
        var proto = std_ObjectCreate(null);
        proto.indexOf = std_ArrayIndexOf;
        proto.join = std_ArrayJoin;
        proto.push = std_ArrayPush;
        proto.slice = std_ArraySlice;
        proto.sort = std_ArraySort;
        List.prototype = proto;
    }
}
MakeConstructible(List);






function Record() {
    return std_ObjectCreate(null);
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
    return std_String(v);
}





function assert(b, info) {
    if (!b)
        AssertionFailed(info);
}

