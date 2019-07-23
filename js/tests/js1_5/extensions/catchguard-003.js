







































var gTestfile = 'catchguard-003.js';

test();

function test()
{
  enterFunc ("test");

  var EXCEPTION_DATA = "String exception";
  var e = "foo", x = "foo";
  var caught = false;

  printStatus ("Catchguard 'Common Scope' test.");
   
  try
  {   
    throw EXCEPTION_DATA;  
  }
  catch (e if ((x = 1) && false))
  {
    reportCompare('PASS', 'FAIL',
		  "Catch block (e if ((x = 1) && false) should not " +
		  "have executed.");
  }
  catch (e if (x == 1))
  {  
    caught = true;
  }
  catch (e)
  {  
    reportCompare('PASS', 'FAIL',
		  "Same scope should be used across all catchguards.");
  }

  if (!caught)
    reportCompare('PASS', 'FAIL',
		  "Exception was never caught.");
   
  if (e != "foo")
    reportCompare('PASS', 'FAIL',
		  "Exception data modified inside catch() scope should " +
		  "not be visible in the function scope (e ='" +
		  e + "'.)");

  if (x != 1)
    reportCompare('PASS', 'FAIL',
		  "Data modified in 'catchguard expression' should " +
		  "be visible in the function scope (x = '" +
		  x + "'.)");

  reportCompare('PASS', 'PASS', 'Catchguard Common Scope test');

  exitFunc ("test");
}
