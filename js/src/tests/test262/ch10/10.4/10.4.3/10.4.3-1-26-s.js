









    
function testcase() {
var obj = new (function () {
    "use strict";
    return this;
});
return (obj !== fnGlobalObject()) && ((typeof obj) !== "undefined");
}
runTestCase(testcase);