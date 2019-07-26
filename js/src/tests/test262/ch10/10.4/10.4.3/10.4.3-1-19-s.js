









    
function testcase() {
"use strict";
var my_eval = eval;
return my_eval("this") === fnGlobalObject();
}
runTestCase(testcase);