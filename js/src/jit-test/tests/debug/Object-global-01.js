

load(libdir + 'asserts.js');

var dbg = new Debugger;
var g = newGlobal();
var gw = dbg.addDebuggee(g);

assertEq(Object.getOwnPropertyDescriptor(gw, 'global'), undefined);
var d = Object.getOwnPropertyDescriptor(Object.getPrototypeOf(gw), 'global');
assertEq(d.enumerable, false);
assertEq(d.configurable, true);
assertEq(typeof d.get, "function");
assertEq(d.get.length, 0);
assertEq(d.set, undefined);


gw.global = '';


assertThrowsInstanceOf(function () { "use strict"; gw.global = {}; }, TypeError);
assertEq(gw.global, gw);


assertThrowsInstanceOf(function () { return Debugger.Object.prototype.global; },
                       TypeError);
