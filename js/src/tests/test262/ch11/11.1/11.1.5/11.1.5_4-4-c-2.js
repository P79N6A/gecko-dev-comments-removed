
















function testcase() {
  try
  {
    eval("({set foo(x){}, foo : 1});");
    return false;
  }
  catch(e)
  {
    return e instanceof SyntaxError;
  }
 }
runTestCase(testcase);
