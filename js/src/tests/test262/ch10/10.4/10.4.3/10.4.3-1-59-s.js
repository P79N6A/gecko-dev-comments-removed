









    
function testcase() {
var o = {};
Object.defineProperty(o, "foo", { get: function() { "use strict"; return this; } });
return o.foo===o;
}
runTestCase(testcase);