









    
function testcase() {
var f = function () {
    "use strict";
    return typeof this;
}
return f() === "undefined";
}
runTestCase(testcase);