










function testcase()
{
  try 
  {
    eval('(function foo(a,a){})');
    return true;
  }
  catch (e) { return false }
  }
runTestCase(testcase);
