









    
function testcase() {
function f() { "use strict"; return this;};
function foo() { return f();}
return foo()===undefined;
}
runTestCase(testcase);