









    
function testcase() {
function f() { "use strict"; return this;};
return f.apply(fnGlobalObject()) === fnGlobalObject();
}
runTestCase(testcase);