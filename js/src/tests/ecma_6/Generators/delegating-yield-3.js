

function* g(n) { for (var i=0; i<n; i++) yield i; }
function* delegate(iter) { return yield* iter; }
var GeneratorObjectPrototype = Object.getPrototypeOf(g).prototype;
var GeneratorObjectPrototype_next = GeneratorObjectPrototype.next;


var inner = g(20);
var outer = delegate(inner);
assertIteratorResult(0, false, outer.next());
assertIteratorResult(1, false, outer.next());
inner.next = function() { return 0; };

assertEq(0, outer.next());

assertEq(0, outer.next());

inner.next = GeneratorObjectPrototype_next;
assertIteratorResult(2, false, outer.next());

inner.next = function() { return { value: 42, done: true }; };
assertIteratorResult(42, true, outer.next());


var inner = g(20);
var outer = delegate(inner);
assertIteratorResult(0, false, outer.next());
assertIteratorResult(1, false, outer.next());
GeneratorObjectPrototype.next = function() { return 0; };

assertEq(0, GeneratorObjectPrototype_next.call(outer));

assertEq(0, GeneratorObjectPrototype_next.call(outer));

GeneratorObjectPrototype.next = GeneratorObjectPrototype_next;
assertIteratorResult(2, false, outer.next());

if (typeof reportCompare == "function")
    reportCompare(true, true);
