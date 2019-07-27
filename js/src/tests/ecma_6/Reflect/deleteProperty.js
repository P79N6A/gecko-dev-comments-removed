



var obj = {x: 1, y: 2};
assertEq(Reflect.deleteProperty(obj, "x"), true);
assertDeepEq(obj, {y: 2});

var arr = [1, 1, 2, 3, 5];
assertEq(Reflect.deleteProperty(arr, "3"), true);
assertDeepEq(arr, [1, 1, 2, , 5]);







assertEq(Reflect.deleteProperty({}, "q"), true);


var proto = {x: 1};
assertEq(Reflect.deleteProperty(Object.create(proto), "x"), true);
assertEq(proto.x, 1);


var arr = [];
assertEq(Reflect.deleteProperty(arr, "length"), false);
assertEq(arr.hasOwnProperty("length"), true);
assertEq(Reflect.deleteProperty(this, "undefined"), false);
assertEq(this.undefined, void 0);


var value;
var proxy = new Proxy({}, {
    deleteProperty(t, k) {
        return value;
    }
});
for (value of [true, false, 0, "something", {}]) {
    assertEq(Reflect.deleteProperty(proxy, "q"), !!value);
}


proxy = new Proxy({}, {
    deleteProperty(t, k) { throw "vase"; }
});
assertThrowsValue(() => Reflect.deleteProperty(proxy, "prop"), "vase");



proxy = new Proxy(Object.freeze({prop: 1}), {
    deleteProperty(t, k) { return true; }
});
assertThrowsInstanceOf(() => Reflect.deleteProperty(proxy, "prop"), TypeError);





function f(x, y, z) {
    assertEq(Reflect.deleteProperty(arguments, "0"), true);
    arguments.x = 33;
    return x;
}
assertEq(f(17, 19, 23), 17);


function testFrozenArguments() {
    Object.freeze(arguments);
    assertEq(Reflect.deleteProperty(arguments, "0"), false);
    assertEq(arguments[0], "zero");
    assertEq(arguments[1], "one");
}
testFrozenArguments("zero", "one");




reportCompare(0, 0);
