













function testcase() {
  'use strict';
  
    var f = Function('arguments = 42;');
    f();
    return true;
}
runTestCase(testcase);