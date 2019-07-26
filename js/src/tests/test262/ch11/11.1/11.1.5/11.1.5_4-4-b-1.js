
















function testcase() {
  try
  {
    eval("({foo : 1, get foo(){}});");
    return false;
  }
  catch(e)
  {
    return e instanceof SyntaxError;
  }
 }
runTestCase(testcase);
