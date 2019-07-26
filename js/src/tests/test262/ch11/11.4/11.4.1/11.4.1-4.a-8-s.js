














function testcase() {
  'use strict';
  
  
  try {
    delete fnGlobalObject().NaN;
    return false;
  }
  catch (e) {
    return (e instanceof TypeError);
  }
 }
runTestCase(testcase);
