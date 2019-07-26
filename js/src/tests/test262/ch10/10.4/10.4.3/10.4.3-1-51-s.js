









    
function testcase() {
return (function () {
    function f() {
        "use strict";
        return typeof this;
    }
    return (f()==="undefined") && (this===fnGlobalObject());
})();
}
runTestCase(testcase);