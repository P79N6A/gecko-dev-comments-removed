









    
function testcase() {
"use strict";
var o = {};
Object.defineProperty(o, "foo",  { get: function() { return this; } });
return o.foo===o;
}
runTestCase(testcase);