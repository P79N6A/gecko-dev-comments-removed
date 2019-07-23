




































var gTestfile = 'regress-356693.js';

var BUGNUMBER = 356693;
var summary = 'Do not assert: pn2->pn_op == JSOP_SETCALL';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'ReferenceError: x is not defined';
  try
  {
    delete (0 ? 3 : x());
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
