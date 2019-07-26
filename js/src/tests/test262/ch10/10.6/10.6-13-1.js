










function testcase() {
  try 
  {
    arguments.caller;
    return true;
  }
  catch (e) {
  }
 }
runTestCase(testcase);
