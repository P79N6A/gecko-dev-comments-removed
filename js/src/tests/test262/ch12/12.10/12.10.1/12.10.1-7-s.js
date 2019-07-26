











function testcase() {
  'use strict';

  try {
    eval("var f = function () {\
                var o = {}; \
                with (o) {}; \
             }\
        ");
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
 }
runTestCase(testcase);
