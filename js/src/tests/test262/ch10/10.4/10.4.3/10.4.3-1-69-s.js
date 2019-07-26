









    
function testcase() {
var o = {};
function f() { "use strict"; return this===o;};
return f.apply(o);
}
runTestCase(testcase);