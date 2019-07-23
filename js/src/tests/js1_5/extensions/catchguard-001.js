







































var gTestfile = 'catchguard-001.js';

test();

function test()
{
  enterFunc ("test");

  var EXCEPTION_DATA = "String exception";
  var e = "foo";
  var caught = false;

  printStatus ("Basic catchguard test.");
   
  try
  {   
    throw EXCEPTION_DATA;  
  }
  catch (e if true)
  {
    caught = true;
    e = "this change should not propagate outside of this scope";
  }
  catch (e if false)
  {  
    reportCompare('PASS', 'FAIL', "Catch block (e if false) should not have executed.");
  }
  catch (e)
  {  
    reportCompare('PASS', 'FAIL', "Catch block (e) should not have executed.");
  }

  if (!caught)
    reportCompare('PASS', 'FAIL', "Exception was never caught.");
   
  if (e != "foo")
    reportCompare('PASS', 'FAIL', "Exception data modified inside catch() scope should " +
		  "not be visible in the function scope (e = '" +
		  e + "'.)");

  reportCompare('PASS', 'PASS', '');
  exitFunc ("test");
}
