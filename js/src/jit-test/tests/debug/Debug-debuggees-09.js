



var dbg = new Debug;

function check(obj) {
    
    assertEq(dbg.hasDebuggee(obj), false);

    
    assertEq(dbg.removeDebuggee(obj), undefined);
}


check(this);
check({});
var g1 = newGlobal('same-compartment');
check(g1);
check(g1.eval("({})"));


var g2 = newGlobal('new-compartment');
g2.parent = this;
g2.eval("var dbg = new Debug(parent);");
check(g2);
check(g2.dbg);
