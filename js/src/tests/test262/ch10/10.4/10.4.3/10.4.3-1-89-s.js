









    
function testcase() {
function f() { return this;};
return (function () {"use strict"; return f.apply(fnGlobalObject()); })() === fnGlobalObject();
}
runTestCase(testcase);