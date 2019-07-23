







































var gTestfile = 'catchguard-002.js';

test();

function test()
{
  enterFunc ("test");

  var EXCEPTION_DATA = "String exception";
  var e;
  var caught = false;

  printStatus ("Basic catchguard test.");
   
  try
  {   
    throw EXCEPTION_DATA;  
  }
  catch (e if true)
  {
    caught = true;
  }
  catch (e if true)
  {  
    reportCompare('PASS', 'FAIL',
		  "Second (e if true) catch block should not have executed.");
  }
  catch (e)
  {  
    reportCompare('PASS', 'FAIL', "Catch block (e) should not have executed.");
  }

  if (!caught)
    reportCompare('PASS', 'FAIL', "Exception was never caught.");
   
  reportCompare('PASS', 'PASS', 'Basic catchguard test');

  exitFunc ("test");
}
