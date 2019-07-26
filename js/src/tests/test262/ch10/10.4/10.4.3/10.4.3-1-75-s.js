









    
function testcase() {
function f() { "use strict"; return this;};
return f.call(fnGlobalObject()) === fnGlobalObject();
}
runTestCase(testcase);