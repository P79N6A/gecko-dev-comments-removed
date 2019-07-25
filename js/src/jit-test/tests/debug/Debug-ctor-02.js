





load(libdir + 'asserts.js');

var g = newGlobal('new-compartment');
g.debuggeeGlobal = this;
g.eval("var dbg = new Debug(debuggeeGlobal);");
assertEq(g.eval("dbg instanceof Debug"), true);



g.parent = this;
assertThrowsInstanceOf(function () { g.eval("parent.Debug(parent.Object())"); }, TypeError);
