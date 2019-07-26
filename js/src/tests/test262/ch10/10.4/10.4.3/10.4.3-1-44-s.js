









    
function testcase() {
return (function () {
    "use strict";
    return ((function () {
        return typeof this;
    })()==="undefined") && ((typeof this)==="undefined");
})();
}
runTestCase(testcase);