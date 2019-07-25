





function C(){}
C.prototype = 1;
try {
    Object.defineProperty(C, "prototype", {get: function() { throw 0; }});
    actual = "no exception";
} catch (exc) {
    actual = exc.name;
}
new C; 
assertEq(actual, "TypeError");
reportCompare(0, 0, "ok");
