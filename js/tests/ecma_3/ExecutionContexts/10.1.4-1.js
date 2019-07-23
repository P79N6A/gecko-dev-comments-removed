







































var gTestfile = '10.1.4-1.js';













var BUGNUMBER = '(none)';
var summary = '10.1.4.1 Entering An Execution Context';
var actual = '';
var expect = '';

test();

function test()
{
  enterFunc ("test");
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var y;
  eval("var x = 1");

  if (delete y)
    reportCompare('PASS', 'FAIL', "Expected *NOT* to be able to delete y");

  if (typeof x == "undefined")
    reportCompare('PASS', 'FAIL', "x did not remain defined after eval()");
  else if (x != 1)
    reportCompare('PASS', 'FAIL', "x did not retain it's value after eval()");
   
  if (!delete x)
    reportCompare('PASS', 'FAIL', "Expected to be able to delete x");

  reportCompare('PASS', 'PASS', '10.1.4.1 Entering An Execution Context');

  exitFunc("test");       
}
