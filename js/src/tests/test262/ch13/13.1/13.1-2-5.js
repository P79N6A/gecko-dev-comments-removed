










function testcase()
{
  try 
  {
    eval("function foo(arguments){};");
    return true;
  }
  catch (e) {  }
 }
runTestCase(testcase);
