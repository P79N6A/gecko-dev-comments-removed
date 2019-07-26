











function testcase() {
  'use strict';

  try {
    eval('for (var eval in null) {};');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
 }
runTestCase(testcase);
