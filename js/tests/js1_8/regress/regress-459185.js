




































var gTestfile = 'regress-459185.js';

var BUGNUMBER = 459185;
var summary = 'Do not assert: pn->pn_arity == PN_BINARY';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  {
    for (var {a: []} = 2 in []) { }
  }
  catch(ex)
  {
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
