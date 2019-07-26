
















function testcase() {
  try
  {
    eval("({foo : 1, set foo(x){}});");
    return false;
  }
  catch(e)
  {
    return e instanceof SyntaxError;
  }
 }
runTestCase(testcase);
