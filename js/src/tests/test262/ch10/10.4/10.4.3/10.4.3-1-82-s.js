









    
function testcase() {
function f() { return this!==undefined;};
return (function () {"use strict"; return eval("f();");})();
}
runTestCase(testcase);