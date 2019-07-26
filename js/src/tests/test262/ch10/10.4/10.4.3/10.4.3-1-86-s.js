









    
function testcase() {
function f() { return this===fnGlobalObject();};
return (function () {"use strict"; return f.apply(null);})();
}
runTestCase(testcase);