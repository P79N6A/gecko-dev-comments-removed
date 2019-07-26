

load(libdir + "asserts.js");

var a = ["A", , "B", "C", "D"];
var normalArrayElementDesc = Object.getOwnPropertyDescriptor(a, 0);
var getterDesc = {
    configurable: false,
    enumerable: true,
    get: function () { return "F"; },
    set: undefined
};
Object.defineProperty(a, 1, getterDesc);



assertThrowsInstanceOf(() => a.sort(), TypeError);


assertDeepEq(Object.getOwnPropertyDescriptor(a, 1), getterDesc);



for (var i = 0; i < a.length; i++) {
    if (i !== 1 && a.hasOwnProperty(i)) {
        normalArrayElementDesc.value = a[i];
        assertDeepEq(Object.getOwnPropertyDescriptor(a, i), normalArrayElementDesc);
    }
}
