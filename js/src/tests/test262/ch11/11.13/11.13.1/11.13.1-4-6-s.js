











function testcase() {
  'use strict';
  
  try {
    Function.length = 42;
    return false;
  }
  catch (e) {
    return (e instanceof TypeError);
  }
 }
runTestCase(testcase);
