
















function testcase() {
  try
  {
    eval("({set foo(arg){}, get foo(){}, set foo(arg1){}});");
    return false;
  }
  catch(e)
  {
    return e instanceof SyntaxError;
  }
 }
runTestCase(testcase);
