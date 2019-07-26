













function testcase()
{
  try 
  {
    eval('"use strict"; var x = "\\43a";');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
}
runTestCase(testcase);