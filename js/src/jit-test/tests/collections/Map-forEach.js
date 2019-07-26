

load(libdir + 'asserts.js');



var testMap = new Map();

function callback(value, key, map) {
    testMap.set(key, value);
    assertEq(map.has(key), true);
    assertEq(map.get(key), value);
}

var initialMap = new Map([['a', 1], ['b', 2.3], [false, undefined]]);
initialMap.forEach(callback);


var iterator = initialMap.iterator();
var count = 0;
for (var [k, v] of testMap) {
    assertEq(initialMap.has(k), true);
    assertEq(initialMap.get(k), testMap.get(k));
    assertEq(iterator.next()[1], testMap.get(k));
    count++;
}


assertEq(initialMap.size, testMap.size);
assertEq(initialMap.size, count);

var x = { abc: 'test'};
function callback2(value, key, map) {
    assertEq(x, this);
}
initialMap = new Map([['a', 1]]);
initialMap.forEach(callback2, x);



var s = new Set([1, 2, 3]);
assertThrowsInstanceOf(function() {
    Map.prototype.forEach.call(s, callback);
}, TypeError, "Map.prototype.forEach should raise TypeError if not used on a Map");

var fn = 2;
assertThrowsInstanceOf(function() {
    initialMap.forEach(fn);
}, TypeError, "Map.prototype.forEach should raise TypeError if callback is not a function");




var m = new Map([["one", 1]]);
Object.getPrototypeOf(m.iterator()).next = function () { throw "FAIL"; };
assertThrowsInstanceOf(function () {
  m.forEach(function () { throw StopIteration; });
}, StopIteration, "Map.prototype.forEach should use intrinsic next method.");
