









    
function testcase() {
var x = 2;
var o = { set foo(stuff) { "use strict"; x=this;  } }
o.foo = 3;
return x===o;
}
runTestCase(testcase);