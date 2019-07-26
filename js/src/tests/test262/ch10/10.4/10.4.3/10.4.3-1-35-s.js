









    
function testcase() {
"use strict";
return (function () {
    return ((function () {
        return typeof this;
    })()==="undefined") && ((typeof this)==="undefined");
})();
}
runTestCase(testcase);