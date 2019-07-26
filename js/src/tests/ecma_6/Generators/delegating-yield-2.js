

function* g1() { return (yield 1); }
function* g2() { try { yield 1; } catch (e) { yield e; } }
function* delegate(iter) { return yield* iter; }
var GeneratorObjectPrototype = Object.getPrototypeOf(g1).prototype;
var GeneratorObjectPrototype_throw = GeneratorObjectPrototype.throw;


var inner = g1();
var outer = delegate(inner);
assertIteratorResult(1, false, outer.next());
assertThrowsValue(function () { outer.throw(42) }, 42);
assertThrowsInstanceOf(function () { outer.throw(42) }, TypeError);


inner = g2();
outer = delegate(inner);
assertIteratorResult(1, false, outer.next());
assertIteratorResult(42, false, outer.throw(42));
assertThrowsValue(function () { outer.throw(42) }, 42);
assertThrowsInstanceOf(function () { outer.throw(42) }, TypeError);


inner = g1();
outer = delegate(inner);
assertIteratorResult(1, false, outer.next());
inner.throw = function(e) { return e*2; };
assertEq(84, outer.throw(42));
assertIteratorResult(undefined, true, outer.next());


inner = g1();
outer = delegate(inner);
inner.next = function() { return { value: 13, done: true } };
assertIteratorResult(13, true, outer.next());


inner = g2();
outer = delegate(inner);
assertIteratorResult(1, false, outer.next());
delete GeneratorObjectPrototype.throw;
var outer_throw_42 = GeneratorObjectPrototype_throw.bind(outer, 42);
assertThrowsValue(outer_throw_42, 42);
assertThrowsInstanceOf(outer_throw_42, TypeError);


inner = g2();
outer = delegate(inner);
outer_throw_42 = GeneratorObjectPrototype_throw.bind(outer, 42);
assertIteratorResult(1, false, outer.next());
GeneratorObjectPrototype.throw = function(e) { return e*2; }
assertEq(84, outer_throw_42());
assertEq(84, outer_throw_42());

assertEq(84, outer_throw_42());
assertIteratorResult(undefined, true, outer.next());


inner = g2();
outer = delegate(inner);
outer_throw_42 = GeneratorObjectPrototype_throw.bind(outer, 42);
assertIteratorResult(1, false, outer.next());
assertEq(84, outer_throw_42());
assertEq(84, outer_throw_42());
GeneratorObjectPrototype.throw = GeneratorObjectPrototype_throw;
assertIteratorResult(42, false, outer_throw_42());
assertIteratorResult(undefined, true, outer.next());

if (typeof reportCompare == "function")
    reportCompare(true, true);
