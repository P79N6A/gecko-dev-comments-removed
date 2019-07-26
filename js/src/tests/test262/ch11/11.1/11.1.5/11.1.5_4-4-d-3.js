
















function testcase() {
  try
  {
    eval("({get foo(){}, set foo(arg){}, get foo(){}});");
    return false;
  }
  catch(e)
  {
    return e instanceof SyntaxError;
  }
 }
runTestCase(testcase);
