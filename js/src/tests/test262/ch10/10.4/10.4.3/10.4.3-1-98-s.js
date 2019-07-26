









    
function testcase() {
var o = {};
function f() { return this===o;};
return (function () {"use strict"; return f.bind(o)();})();
}
runTestCase(testcase);