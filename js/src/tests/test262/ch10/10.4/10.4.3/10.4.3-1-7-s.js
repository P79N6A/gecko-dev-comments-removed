











function testcase() {
"use strict";
function f() {
    return typeof this;
}
return f() === "undefined";
}
runTestCase(testcase);
