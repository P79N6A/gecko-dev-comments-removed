

function* g1() { return (yield 1); }
function* g2() { try { yield 1; } catch (e) { yield e; } }
function* delegate(iter) { return yield* iter; }
var GeneratorObjectPrototype = Object.getPrototypeOf(g1).prototype;
var GeneratorObjectPrototype_throw = GeneratorObjectPrototype.throw;


var inner = g1();
var outer = delegate(inner);
assertIteratorResult(outer.next(), 1, false);
assertThrowsValue(function () { outer.throw(42) }, 42);
assertThrowsInstanceOf(function () { outer.throw(42) }, TypeError);


inner = g2();
outer = delegate(inner);
assertIteratorResult(outer.next(), 1, false);
assertIteratorResult(outer.throw(42), 42, false);
assertThrowsValue(function () { outer.throw(42) }, 42);
assertThrowsInstanceOf(function () { outer.throw(42) }, TypeError);


inner = g1();
outer = delegate(inner);
assertIteratorResult(outer.next(), 1, false);
inner.throw = function(e) { return e*2; };
assertEq(84, outer.throw(42));
assertIteratorResult(outer.next(), undefined, true);


inner = g1();
outer = delegate(inner);
inner.next = function() { return { value: 13, done: true } };
assertIteratorResult(outer.next(), 13, true);


inner = g2();
outer = delegate(inner);
assertIteratorResult(outer.next(), 1, false);
delete GeneratorObjectPrototype.throw;
var outer_throw_42 = GeneratorObjectPrototype_throw.bind(outer, 42);
assertThrowsValue(outer_throw_42, 42);
assertThrowsInstanceOf(outer_throw_42, TypeError);


inner = g2();
outer = delegate(inner);
outer_throw_42 = GeneratorObjectPrototype_throw.bind(outer, 42);
assertIteratorResult(outer.next(), 1, false);
GeneratorObjectPrototype.throw = function(e) { return e*2; }
assertEq(84, outer_throw_42());
assertEq(84, outer_throw_42());

assertEq(84, outer_throw_42());
assertIteratorResult(outer.next(), undefined, true);


inner = g2();
outer = delegate(inner);
outer_throw_42 = GeneratorObjectPrototype_throw.bind(outer, 42);
assertIteratorResult(outer.next(), 1, false);
assertEq(84, outer_throw_42());
assertEq(84, outer_throw_42());
GeneratorObjectPrototype.throw = GeneratorObjectPrototype_throw;
assertIteratorResult(outer_throw_42(), 42, false);
assertIteratorResult(outer.next(), undefined, true);

if (typeof reportCompare == "function")
    reportCompare(true, true);
