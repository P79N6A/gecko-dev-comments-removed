









    
function testcase() {
"use strict";
var f = Function("return typeof this;");
return f() !== "undefined";
}
runTestCase(testcase);