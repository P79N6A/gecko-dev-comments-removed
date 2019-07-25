


load(libdir + "asserts.js");


var dbg = new Debugger;
assertThrowsInstanceOf(function () { dbg.addDebuggee(this); }, TypeError);
assertThrowsInstanceOf(function () { new Debugger(this); }, TypeError);


var d1 = newGlobal('new-compartment');
d1.top = this;
d1.eval("var dbg = new Debugger(top)");
assertThrowsInstanceOf(function () { dbg.addDebuggee(d1); }, TypeError);
assertThrowsInstanceOf(function () { new Debugger(d1); }, TypeError);


var d2 = newGlobal('new-compartment');
d2.top = this;
d2.eval("var dbg = new Debugger(top.d1)");
assertThrowsInstanceOf(function () { dbg.addDebuggee(d2); }, TypeError);
assertThrowsInstanceOf(function () { new Debugger(d2); }, TypeError);
