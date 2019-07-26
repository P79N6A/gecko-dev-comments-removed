











function testcase() {
  try {
    
    
    eval("\
            function foo() {\
                function f() {\
                  \'use strict\'; \
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
