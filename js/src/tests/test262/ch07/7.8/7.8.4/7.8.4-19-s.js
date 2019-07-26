













function testcase()
{
  try 
  {
    eval('"use strict"; var x = "\\463";');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
}
runTestCase(testcase);