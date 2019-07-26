









    
function testcase() {
function f() { "use strict"; return this===undefined;};
return f.bind(undefined)();
}
runTestCase(testcase);