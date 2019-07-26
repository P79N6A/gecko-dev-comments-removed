











function testcase() {
    'use strict';

    try {
      fnGlobalObject().Infinity = 42;
      return false;
    }
    catch (e) {
      return (e instanceof TypeError);
    }
 }
runTestCase(testcase);
