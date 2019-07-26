

load(libdir + 'asserts.js');



var testSet = new Set();

function callback(value, key, set) {
    assertEq(value, key);
    testSet.add(value);
    assertEq(set.has(key), true);
}

var initialSet = new Set(['a', 1, undefined]);
initialSet.forEach(callback);


var iterator = initialSet.iterator();
var count = 0;
for (var v of testSet) {
    assertEq(initialSet.has(v), true);
    assertEq(iterator.next(), v);
    count++;
}


assertEq(initialSet.size, testSet.size);
assertEq(initialSet.size, count);

var x = { abc: 'test'};
function callback2(value, key, set) {
    assertEq(x, this);
}
initialSet = new Set(['a']);
initialSet.forEach(callback2, x);



var m = new Map([['a', 1], ['b', 2.3], ['c', undefined]]);
assertThrowsInstanceOf(function() {
    Set.prototype.forEach.call(m, callback);
}, TypeError, "Set.prototype.forEach should raise TypeError if not a used on a Set");

var fn = 2;
assertThrowsInstanceOf(function() {
    initialSet.forEach(fn);
}, TypeError, "Set.prototype.forEach should raise TypeError if callback is not a function");




var s = new Set(["one", 1]);
Object.getPrototypeOf(s.iterator()).next = function () { throw "FAIL"; };
assertThrowsInstanceOf(function () {
  s.forEach(function () { throw StopIteration; });
}, StopIteration, "Set.prototype.forEach should use intrinsic next method.");
