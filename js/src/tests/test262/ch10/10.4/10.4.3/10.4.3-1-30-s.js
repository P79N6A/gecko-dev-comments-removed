









    
function testcase() {
"use strict";
var f1 = function () {
    function f() {
        return typeof this;
    }
    return (f()==="undefined") && ((typeof this)==="undefined");
}
return f1();
}
runTestCase(testcase);