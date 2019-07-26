

function* g1() { return (yield 1); }
function* g2() { try { yield 1; } catch (e) { yield e; } }
function* delegate(iter) { return yield* iter; }
var GeneratorObjectPrototype = Object.getPrototypeOf(g1).prototype;
var GeneratorObjectPrototype_throw = GeneratorObjectPrototype.throw;


var inner = g1();
var outer = delegate(inner);
assertIteratorNext(outer, 1);
assertThrowsValue(function () { outer.throw(42) }, 42);
assertThrowsValue(function () { outer.throw(42) }, 42);


inner = g2();
outer = delegate(inner);
assertIteratorNext(outer, 1);
assertIteratorResult(outer.throw(42), 42, false);
assertThrowsValue(function () { outer.throw(42) }, 42);
assertThrowsValue(function () { outer.throw(42) }, 42);


inner = g1();
outer = delegate(inner);
assertIteratorNext(outer, 1);
inner.throw = function(e) { return e*2; };
assertEq(84, outer.throw(42));
assertIteratorDone(outer, undefined);


inner = g1();
outer = delegate(inner);
inner.next = function() { return { value: 13, done: true } };
assertIteratorDone(outer, 13);


inner = g2();
outer = delegate(inner);
assertIteratorNext(outer, 1);
delete GeneratorObjectPrototype.throw;
var outer_throw_42 = GeneratorObjectPrototype_throw.bind(outer, 42);
assertThrowsValue(outer_throw_42, 42);
assertThrowsValue(outer_throw_42, 42);


inner = g2();
outer = delegate(inner);
outer_throw_42 = GeneratorObjectPrototype_throw.bind(outer, 42);
assertIteratorNext(outer, 1);
GeneratorObjectPrototype.throw = function(e) { return e*2; }
assertEq(84, outer_throw_42());
assertEq(84, outer_throw_42());

assertEq(84, outer_throw_42());
assertIteratorDone(outer, undefined);


inner = g2();
outer = delegate(inner);
outer_throw_42 = GeneratorObjectPrototype_throw.bind(outer, 42);
assertIteratorNext(outer, 1);
assertEq(84, outer_throw_42());
assertEq(84, outer_throw_42());
GeneratorObjectPrototype.throw = GeneratorObjectPrototype_throw;
assertIteratorResult(outer_throw_42(), 42, false);
assertIteratorDone(outer, undefined);

if (typeof reportCompare == "function")
    reportCompare(true, true);
