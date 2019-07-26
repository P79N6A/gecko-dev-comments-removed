














function testcase() {
  'use strict';
  
  try {
    delete Math.LN2;
    return false;
  }
  catch (e) {
    return (e instanceof TypeError); 
  }
 }
runTestCase(testcase);
