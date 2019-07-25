



assertThrows(function () { Debug(); }, TypeError);
assertThrows(function () { Debug(null); }, TypeError);
assertThrows(function () { Debug(true); }, TypeError);
assertThrows(function () { Debug(42); }, TypeError);
assertThrows(function () { Debug("bad"); }, TypeError);
assertThrows(function () { Debug(function () {}); }, TypeError);
assertThrows(function () { Debug(this); }, TypeError);
assertThrows(function () { new Debug(); }, TypeError);
assertThrows(function () { new Debug(null); }, TypeError);
assertThrows(function () { new Debug(true); }, TypeError);
assertThrows(function () { new Debug(42); }, TypeError);
assertThrows(function () { new Debug("bad"); }, TypeError);
assertThrows(function () { new Debug(function () {}); }, TypeError);
assertThrows(function () { new Debug(this); }, TypeError);


var g = newGlobal('new-compartment');
var dbg = new Debug(g);
assertEq(dbg instanceof Debug, true);
assertEq(Object.getPrototypeOf(dbg), Debug.prototype);


var g2 = newGlobal('new-compartment');
g2.debuggeeGlobal = this;
g2.eval("var dbg = new Debug(debuggeeGlobal);");
assertEq(g2.eval("dbg instanceof Debug"), true);



g2.outer = this;
assertThrows(function () { g2.eval("outer.Debug(outer.Object())"); }, TypeError);

reportCompare(0, 0, 'ok');
