

load(libdir + 'asserts.js');


assertThrowsInstanceOf(function () { Debug(); }, TypeError);
assertThrowsInstanceOf(function () { Debug(null); }, TypeError);
assertThrowsInstanceOf(function () { Debug(true); }, TypeError);
assertThrowsInstanceOf(function () { Debug(42); }, TypeError);
assertThrowsInstanceOf(function () { Debug("bad"); }, TypeError);
assertThrowsInstanceOf(function () { Debug(function () {}); }, TypeError);
assertThrowsInstanceOf(function () { Debug(this); }, TypeError);
assertThrowsInstanceOf(function () { new Debug(); }, TypeError);
assertThrowsInstanceOf(function () { new Debug(null); }, TypeError);
assertThrowsInstanceOf(function () { new Debug(true); }, TypeError);
assertThrowsInstanceOf(function () { new Debug(42); }, TypeError);
assertThrowsInstanceOf(function () { new Debug("bad"); }, TypeError);
assertThrowsInstanceOf(function () { new Debug(function () {}); }, TypeError);
assertThrowsInstanceOf(function () { new Debug(this); }, TypeError);


var g = newGlobal('new-compartment');
var dbg = new Debug(g);
assertEq(dbg instanceof Debug, true);
assertEq(Object.getPrototypeOf(dbg), Debug.prototype);


var g2 = newGlobal('new-compartment');
g2.debuggeeGlobal = this;
g2.eval("var dbg = new Debug(debuggeeGlobal);");
assertEq(g2.eval("dbg instanceof Debug"), true);



g2.outer = this;
assertThrowsInstanceOf(function () { g2.eval("outer.Debug(outer.Object())"); }, TypeError);
