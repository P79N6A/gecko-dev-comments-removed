

load(libdir + "asserts.js");

function f() {
    var v = new Debug;
    var g = newGlobal('new-compartment');
    v.addDebuggee(g); 
}

assertThrowsInstanceOf(f, Error);
