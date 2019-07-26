









    
function testcase() {
"use strict";
return (function () {
    return typeof this;
})() === "undefined";
}
runTestCase(testcase);