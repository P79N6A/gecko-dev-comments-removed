









    
function testcase() {
"use strict";
var f1 = function () {
    var f = function () {
        return typeof this;
    }
    return (f()==="undefined") && ((typeof this)==="undefined");
}
return f1();
}
runTestCase(testcase);