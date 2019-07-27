var g = newGlobal();
var f1 = g.evaluate("(function (x) { function inner() {}; })");
gczeal(2, 1); 
var f2 = clone(f1);
