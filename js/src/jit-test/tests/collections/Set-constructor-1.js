

load(libdir + "asserts.js");

var s = new Set();
assertEq(s.size, 0);
s = new Set(undefined);
assertEq(s.size, 0);
s = new Set(null);
assertEq(s.size, 0);


options("werror");
assertEq(evaluate("Set()", {catchTermination: true}), "terminated");



