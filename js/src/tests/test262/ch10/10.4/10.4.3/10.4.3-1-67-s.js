









    
function testcase() {
function f() { "use strict"; return this===null;};
return f.apply(null);
}
runTestCase(testcase);