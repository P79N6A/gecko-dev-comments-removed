









    
function testcase() {
"use strict";
return (eval("typeof this") === "undefined") && (eval("this") !== fnGlobalObject());
}
runTestCase(testcase);