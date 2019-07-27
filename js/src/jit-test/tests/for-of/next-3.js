





load(libdir + "asserts.js");
load(libdir + "iteration.js");

var g = newGlobal();
g.eval("var it = [1, 2]['" + std_iterator + "']();");
assertIteratorNext(g.it, 1);
assertThrowsInstanceOf([][std_iterator]().next.bind(g.it), TypeError)
