









    
function testcase() {
function f() { "use strict"; return this===undefined;};
return f.apply(undefined);
}
runTestCase(testcase);