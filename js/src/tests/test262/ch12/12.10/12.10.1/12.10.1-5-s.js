











function testcase() {
  
    Function("\'use strict\'; var f1 = Function( \"var o = {}; with (o) {};\")");
    return true;
  
 }
runTestCase(testcase);
