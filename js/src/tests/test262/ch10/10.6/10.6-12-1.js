










function testcase() {
  try 
  {
    arguments.callee;
    return true;
  }
  catch (e) {
  }
 }
runTestCase(testcase);
