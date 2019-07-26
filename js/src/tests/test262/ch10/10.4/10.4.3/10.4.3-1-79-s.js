









    
function testcase() {
var o = {};
function f() { "use strict"; return this===o;};
return f.bind(o)();
}
runTestCase(testcase);