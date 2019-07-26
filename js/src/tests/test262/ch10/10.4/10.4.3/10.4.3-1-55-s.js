









    
function testcase() {
var o = { get foo() { "use strict"; return this; } }
return o.foo===o;
}
runTestCase(testcase);