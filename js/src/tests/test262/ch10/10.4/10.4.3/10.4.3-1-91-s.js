









    
function testcase() {
function f() { return this===fnGlobalObject();};
return (function () {"use strict"; return f.call(null); })();
}
runTestCase(testcase);