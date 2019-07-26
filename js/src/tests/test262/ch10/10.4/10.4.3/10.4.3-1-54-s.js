









    
function testcase() {
"use strict";
var o = { get foo() { return this; } }
return o.foo===o;
}
runTestCase(testcase);