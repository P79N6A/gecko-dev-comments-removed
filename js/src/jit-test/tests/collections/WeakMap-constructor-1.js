

load(libdir + "asserts.js");

new WeakMap();
new WeakMap(undefined);
new WeakMap(null);


options("werror");
assertEq(evaluate("WeakMap()", {catchTermination: true}), "terminated");



