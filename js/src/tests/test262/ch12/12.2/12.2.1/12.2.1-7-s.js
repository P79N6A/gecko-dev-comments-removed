











function testcase() {
  'use strict';

  try {
    eval('var eval;');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
 }
runTestCase(testcase);
