





DESCRIPTION = "Illegally constructed catchguard should have thrown an exception.";
EXPECTED = "error";

var expect;
var actual;

test();

function test()
{
  enterFunc ("test");

  var EXCEPTION_DATA = "String exception";
  var e;

  printStatus ("Catchguard syntax negative test #2.");
   
  try
  {   
    throw EXCEPTION_DATA;  
  }
  catch (e)
  {  
    actual = e + ': 1';
  }
  catch (e) 
  {
    actual = e + ': 2';
  }

  reportCompare(expect, actual, DESCRIPTION);

  exitFunc ("test");
}
