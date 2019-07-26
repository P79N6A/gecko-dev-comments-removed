











function testcase() {
function f() {
    "use strict";
    return typeof this;
}
return f() === "undefined";
}
runTestCase(testcase);
