









    
function testcase() {
var f = new Function("\"use strict\";\nreturn typeof this;");
return f() === "undefined";
}
runTestCase(testcase);