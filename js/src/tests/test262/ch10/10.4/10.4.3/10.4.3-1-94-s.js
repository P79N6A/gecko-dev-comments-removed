









    
function testcase() {
function f() { return this===fnGlobalObject();};
return (function () {"use strict"; return f.call(fnGlobalObject());})();
}
runTestCase(testcase);