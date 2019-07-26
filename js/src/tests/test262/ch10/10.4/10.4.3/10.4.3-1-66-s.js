









    
function testcase() {
function f() { "use strict"; return this===undefined;};
return f.apply();
}
runTestCase(testcase);