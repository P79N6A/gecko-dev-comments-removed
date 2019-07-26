









    
function testcase() {
fnGlobalObject().f = function()  { "use strict"; return this===undefined;};
return (new Function("return f();"))();
}
runTestCase(testcase);