













function testcase()
{
  try 
  {
    eval('"use strict"; var x = "\\411";');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
}
runTestCase(testcase);