









    
function testcase() {
"use strict";
var f = function () {
    return typeof this;
}
return f() === "undefined";
}
runTestCase(testcase);