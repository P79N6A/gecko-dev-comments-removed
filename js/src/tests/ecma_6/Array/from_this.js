




var hits = 0, obj = {};
function f(x) {
    assertEq(this, obj);
    hits++;
}
Array.from(["a", "b", "c"], f, obj);
assertEq(hits, 3);


hits = 0;
function gs(x) {
    "use strict";
    assertEq(this, undefined);
    hits++;
}
Array.from("def", gs);
assertEq(hits, 3);



var global = this;
hits = 0;
function g(x) {
    assertEq(this, global);
    hits++;
}
Array.from("ghi", g);
assertEq(hits, 3);


for (var v of [0, "str", undefined]) {
    hits = 0;
    var mapfn = function h(x) {
        "use strict";
        assertEq(this, v);
        hits++;
    };
    Array.from("pq", mapfn, v);
    assertEq(hits, 2);
}

if (typeof reportCompare === 'function')
    reportCompare(0, 0);
