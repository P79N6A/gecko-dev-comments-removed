









    
function testcase() {
function f() { return this!==undefined;};
function foo() { "use strict"; return f();}
return foo();
}
runTestCase(testcase);