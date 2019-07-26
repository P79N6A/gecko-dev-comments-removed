









    
function testcase() {
function f() { return this!==undefined;};
return (function () {"use strict"; return f.apply();})();
}
runTestCase(testcase);