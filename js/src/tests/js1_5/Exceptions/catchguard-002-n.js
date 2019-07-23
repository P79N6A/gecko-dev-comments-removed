







































var gTestfile = 'catchguard-002-n.js';

DESCRIPTION = "var in catch clause should have caused an error.";
EXPECTED = "error";

var expect;
var actual;

test();

function test()
{
  enterFunc ("test");

  var EXCEPTION_DATA = "String exception";
  var e;

  printStatus ("Catchguard var declaration negative test.");
   
  try
  {   
    throw EXCEPTION_DATA;  
  }
  catch (var e)
  {  
    actual = e + '';
  }

  reportCompare(expect, actual, DESCRIPTION);

  exitFunc ("test");
}
