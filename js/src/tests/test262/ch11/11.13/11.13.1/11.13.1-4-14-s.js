











function testcase() {
  'use strict';

  try {
    Number.MAX_VALUE = 42;
    return false;
  }
  catch (e) {
    return (e instanceof TypeError);
  }
 }
runTestCase(testcase);
