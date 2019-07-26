









    
function testcase() {
return (function () {
    "use strict";
    var f = function () {
        return typeof this;
    }
    return (f()==="undefined") && ((typeof this)==="undefined");
})();
}
runTestCase(testcase);