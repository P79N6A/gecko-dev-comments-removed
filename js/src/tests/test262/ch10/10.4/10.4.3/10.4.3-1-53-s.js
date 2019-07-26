









    
function testcase() {
return (function () {
    return ((function () {
        "use strict";
        return typeof this;
    })()==="undefined") && (this===fnGlobalObject());
})();
}
runTestCase(testcase);