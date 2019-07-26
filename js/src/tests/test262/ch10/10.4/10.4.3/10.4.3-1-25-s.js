









    
function testcase() {
"use strict";
var obj = new (function () {
    return this;
});
return (obj !== fnGlobalObject()) && ((typeof obj) !== "undefined");
}
runTestCase(testcase);