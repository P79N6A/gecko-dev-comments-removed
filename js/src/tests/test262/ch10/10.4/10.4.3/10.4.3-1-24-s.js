









    
function testcase() {
var f = function () {
    "use strict";
    return this;
}
return ( (new f())!==fnGlobalObject()) && (typeof (new f()) !== "undefined");
}
runTestCase(testcase);