









    
function testcase() {
var my_eval = eval;
return my_eval("\"use strict\";\nthis") === fnGlobalObject();
}
runTestCase(testcase);