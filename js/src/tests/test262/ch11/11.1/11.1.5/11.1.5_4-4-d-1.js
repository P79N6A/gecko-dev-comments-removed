
















function testcase() {
  try
  {
    eval("({get foo(){}, get foo(){}});");
    return false;
  }
  catch(e)
  {
    return e instanceof SyntaxError;
  }
 }
runTestCase(testcase);
