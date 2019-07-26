











function testcase() {
  'use strict';
  
    var f = Function('eval = 42;');
    f();
    return true;
 }
runTestCase(testcase);
