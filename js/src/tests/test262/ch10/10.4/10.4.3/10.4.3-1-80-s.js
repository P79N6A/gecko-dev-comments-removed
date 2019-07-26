









    
function testcase() {
function f() { "use strict"; return this;};
return f.bind(fnGlobalObject())() === fnGlobalObject();
}
runTestCase(testcase);