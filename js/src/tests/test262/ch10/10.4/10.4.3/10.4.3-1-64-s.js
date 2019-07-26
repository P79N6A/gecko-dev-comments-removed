









    
function testcase() {
fnGlobalObject().f = function() { "use strict"; return this===undefined;};
return Function("return f();")();
}
runTestCase(testcase);