








setRestoreFunction((function () {
    var Array_indexOf = Array.prototype.indexOf;
    var Array_join = Array.prototype.join;
    var Array_push = Array.prototype.push;
    var Array_slice = Array.prototype.slice;
    var Array_sort = Array.prototype.sort;
    return function () {
        delete Array.prototype["0"];
        Array.prototype.indexOf = Array_indexOf;
        Array.prototype.join = Array_join;
        Array.prototype.push = Array_push;
        Array.prototype.slice = Array_slice;
        Array.prototype.sort = Array_sort;
    };
}()));






include("test402/lib/testBuiltInObject.js");
include("test402/lib/testIntl.js");






function $INCLUDE(file) {
}
