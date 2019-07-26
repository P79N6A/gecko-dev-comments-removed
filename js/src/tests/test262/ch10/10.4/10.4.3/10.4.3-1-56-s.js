









    
function testcase() {
"use strict";
var x = 2;
var o = { set foo(stuff) { x=this; } }
o.foo = 3;
return x===o;
}
runTestCase(testcase);