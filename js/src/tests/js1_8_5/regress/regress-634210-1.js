




function f() {}
f.p = function() {};
Object.freeze(f);
f.p;

reportCompare(0, 0, "ok");
