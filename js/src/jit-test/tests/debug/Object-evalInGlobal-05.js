

load(libdir + 'asserts.js');

var dbg = new Debugger();

var g1 = newGlobal();
var dg1 = dbg.addDebuggee(g1);

var g2 = newGlobal();
var dg2 = dbg.addDebuggee(g2);


var dg1wg2 = dg1.makeDebuggeeValue(g2);
assertEq(dg1wg2.global, dg1);
assertEq(dg1wg2.unwrap(), dg2);
assertThrowsInstanceOf(function () { dg1wg2.evalInGlobal('1'); }, TypeError);
assertThrowsInstanceOf(function () { dg1wg2.evalInGlobalWithBindings('x', { x: 1 }); }, TypeError);


assertEq(dg1wg2.unwrap().evalInGlobal('1729').return, 1729);
assertEq(dg1wg2.unwrap().evalInGlobalWithBindings('x', { x: 1729 }).return, 1729);
