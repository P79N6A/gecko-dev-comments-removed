











function testcase() {
  try {
    
    
    eval("\
          function foo() {\
            \'use strict\'; \
            function f() {\
                var o = {}; \
                with (o) {};\
            }\
          }\
        ");
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
 }
runTestCase(testcase);
