
















function testcase() {
  try
  {
    eval("({get foo(){}, foo : 1});");
    return false;
  }
  catch(e)
  {
    return e instanceof SyntaxError;
  }
 }
runTestCase(testcase);
