


load(libdir + "asserts.js");
var g = newGlobal();
g.__proto__ = {};
assertThrowsInstanceOf(() => g.eval("s += ''"), g.ReferenceError);
