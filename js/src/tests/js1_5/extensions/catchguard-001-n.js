







































var gTestfile = 'catchguard-001-n.js';

DESCRIPTION = " the non-guarded catch should HAVE to appear last";
EXPECTED = "error";

test();

function test()
{
  enterFunc ("test");

  var EXCEPTION_DATA = "String exception";
  var e;

  printStatus ("Catchguard syntax negative test.");
   
  try
  {   
    throw EXCEPTION_DATA;  
  }
  catch (e) 
  {  

  }
  catch (e if true)
  {

  }
  catch (e if false)
  {  

  }

  reportCompare('PASS', 'FAIL',
		"Illegally constructed catchguard should have thrown " +
		"an exception.");

  exitFunc ("test");
}
