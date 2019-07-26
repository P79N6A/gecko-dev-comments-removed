


function* g(n) { for (var i=0; i<n; i++) yield i; }
function* delegate(iter) { return yield* iter; }

var log = "", inner, outer;


var Poison = new Error;

function log_calls(method) {
    return function () {
        log += "x"
        return method.call(this);
    }
}

function poison(receiver, prop) {
    Object.defineProperty(receiver, prop, { get: function () { throw Poison } });
}


inner = g(10);
outer = delegate(inner);
inner.throw = log_calls(inner.throw);
poison(inner, 'next')
assertThrowsValue(outer.next.bind(outer), Poison);
assertEq(log, "");


inner = g(10);
outer = delegate(inner);
inner.next = function () { return { done: true, get value() { throw Poison} } };
inner.throw = log_calls(inner.throw);
assertThrowsValue(outer.next.bind(outer), Poison);
assertEq(log, "");


inner = g(10);
outer = delegate(inner);
inner.next = function () { return { get done() { throw Poison }, value: 42 } };
inner.throw = log_calls(inner.throw);
assertThrowsValue(outer.next.bind(outer), Poison);
assertEq(log, "");


if (typeof reportCompare == "function")
    reportCompare(true, true);
