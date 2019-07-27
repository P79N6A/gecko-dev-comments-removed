

load(libdir + "asserts.js");

var m = new Map();
assertEq(m.size, 0);
m = new Map(undefined);
assertEq(m.size, 0);
m = new Map(null);
assertEq(m.size, 0);


options("werror");
assertEq(evaluate("Map()", {catchTermination: true}), "terminated");



