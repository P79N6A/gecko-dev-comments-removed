









    
function testcase() {
function f() { "use strict"; return this===undefined;};
return f.call();
}
runTestCase(testcase);