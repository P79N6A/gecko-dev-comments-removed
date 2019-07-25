


var g = newGlobal('new-compartment');
var obj = g.eval("Object.create(null)");
var dbg = new Debug;


assertEq(dbg.hasDebuggee(obj), false);


var added;
try {
    dbg.addDebuggee(obj);
    added = true;
} catch (exc) {
    if (!(exc instanceof TypeError))
        throw exc;
    added = false;
}

assertEq(dbg.hasDebuggee(obj), added);
assertEq(dbg.getDebuggees().length, added ? 1 : 0);
assertEq(dbg.removeDebuggee(obj), undefined);
assertEq(dbg.hasDebuggee(obj), false);
