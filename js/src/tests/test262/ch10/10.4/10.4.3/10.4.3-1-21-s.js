









    
function testcase() {
"use strict";
function f() {
    return this;
}
return ( (new f())!==fnGlobalObject()) && (typeof (new f()) !== "undefined");
}
runTestCase(testcase);