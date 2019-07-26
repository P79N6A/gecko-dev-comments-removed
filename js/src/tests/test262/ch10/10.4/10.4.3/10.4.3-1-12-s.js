









    
function testcase() {
return (function () {
    "use strict";
    return typeof this;
})() === "undefined";
}
runTestCase(testcase);