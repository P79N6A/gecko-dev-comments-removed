









    
function testcase() {
function f() { "use strict"; return this===null;};
return f.bind(null)();
}
runTestCase(testcase);