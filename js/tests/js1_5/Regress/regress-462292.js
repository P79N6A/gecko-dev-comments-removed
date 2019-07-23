




































var gTestfile = 'regress-462292.js';

var BUGNUMBER = 462292;
var summary = 'Do not assert: pn->pn_op == JSOP_CALL || pn->pn_op == JSOP_EVAL';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  try
  {
    [].apply() = 1;
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
