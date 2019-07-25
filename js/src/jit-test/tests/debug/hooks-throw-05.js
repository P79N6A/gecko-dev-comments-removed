


var g = newGlobal('new-compartment');
g.parent = this;
g.eval("new Debugger(parent).hooks = {throw: function () {}};");

var obj = new Error("oops");
try {
    throw obj;
} catch (exc) {
    assertEq(exc, obj);
}
