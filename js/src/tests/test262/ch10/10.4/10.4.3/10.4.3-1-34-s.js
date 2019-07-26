









    
function testcase() {
"use strict";
return (function () {
    var f = function () {
        return typeof this;
    }
    return (f()==="undefined") && ((typeof this)==="undefined");
})();
}
runTestCase(testcase);