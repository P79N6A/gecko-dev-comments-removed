







































var gTestfile = '10.1.3.js';







test();

function f()
{
  var x;

  return typeof x;

  function x()
  {
    return 7;   
  }
}

function test()
{
  enterFunc ("test");

  printStatus ("ECMA Section: 10.1.3: Variable Instantiation.");
  printBugNumber (17290);

  reportCompare ("function", f(), "Declaration precedence test");

  exitFunc("test");       
}
