









    
function testcase() {
function f() { "use strict"; return this===undefined;};
return f.call(undefined);
}
runTestCase(testcase);