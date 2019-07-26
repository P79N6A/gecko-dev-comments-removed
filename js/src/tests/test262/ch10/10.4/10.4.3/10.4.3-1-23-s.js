









    
function testcase() {
"use strict";
var f = function () {
    return this;
}
return ( (new f())!==fnGlobalObject()) && (typeof (new f()) !== "undefined");

}
runTestCase(testcase);