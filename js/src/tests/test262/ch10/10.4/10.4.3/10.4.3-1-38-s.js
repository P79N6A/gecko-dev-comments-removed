









    
function testcase() {
function f1() {
    "use strict";
    return ((function () {
        return typeof this;
    })()==="undefined") && ((typeof this)==="undefined");
}
return f1();
}
runTestCase(testcase);