









    
function testcase() {
function f1() {
    "use strict";
    var f = function () {
        return typeof this;
    }
    return (f()==="undefined") && ((typeof this)==="undefined");
}
return f1();
}
runTestCase(testcase);