









    
function testcase() {
function f() { "use strict"; return this===null;};
return f.call(null);
}
runTestCase(testcase);