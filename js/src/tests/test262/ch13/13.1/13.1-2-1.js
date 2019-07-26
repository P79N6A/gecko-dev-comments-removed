










function testcase()
{
  try 
  {
    eval("function foo(eval){};");
    return true;
  }
  catch (e) {  }
 }
runTestCase(testcase);
