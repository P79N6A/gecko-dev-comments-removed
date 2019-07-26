











function testcase() {
  'use strict';

  try {
    eval('(function () { var eval; })');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
 }
runTestCase(testcase);
