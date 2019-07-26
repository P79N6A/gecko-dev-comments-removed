











function testcase() {
  'use strict';
  var s = eval;
  s('eval = 42;');
  return true;
 }
runTestCase(testcase);
