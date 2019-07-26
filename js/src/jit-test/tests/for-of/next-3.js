





load(libdir + "asserts.js");
load(libdir + "iteration.js");

var g = newGlobal();
g.eval("var it = [1, 2]['" + std_iterator + "']();");
assertIteratorResult(g.it.next(), 1, false);
assertThrowsInstanceOf([][std_iterator]().next.bind(g.it), TypeError)
