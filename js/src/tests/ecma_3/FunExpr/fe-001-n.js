







































var gTestfile = 'fe-001-n.js';

DESCRIPTION = "Previous statement should have thrown a ReferenceError";
EXPECTED = "error";

test();

function test()
{
  enterFunc ("test");
  printStatus ("Function Expression test.");

  var x = function f(){return "inner";}();
  var y = f();   
  reportCompare('PASS', 'FAIL', "Previous statement should have thrown a ReferenceError");

  exitFunc ("test");
}
