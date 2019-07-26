









    
function testcase() {
function f() { "use strict"; return this===undefined;};
return eval("f();");
}
runTestCase(testcase);