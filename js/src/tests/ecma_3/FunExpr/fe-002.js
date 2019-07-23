







































var gTestfile = 'fe-002.js';

function f()
{
  return "outer";
}

function test()
{
  enterFunc ("test");
  printStatus ("Function Expression test.");

  var x = function f(){return "inner";}();
   
  reportCompare ("outer", f(),
		 "Inner function statement should not have been called.");
   
  exitFunc ("test");
}

test();
