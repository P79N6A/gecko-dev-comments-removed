











function testcase() {
  try {
    var f = Function("\
                      \'use strict\';  \
                      var o = {}; \
                      with (o) {};\
                    ");
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
	
  }
 }
runTestCase(testcase);
