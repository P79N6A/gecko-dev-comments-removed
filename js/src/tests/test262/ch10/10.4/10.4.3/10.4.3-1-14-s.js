









    
function testcase() {
var f = Function("\"use strict\";\nreturn typeof this;");
return f() === "undefined";
}
runTestCase(testcase);