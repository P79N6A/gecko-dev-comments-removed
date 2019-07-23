




































var gTestfile = 'regress-350279.js';

var BUGNUMBER = 350279;
var summary = 'Do not assert: left->pn_type == TOK_RC';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = /SyntaxError: /;
  try
  {
    eval('let [2 for (x in [])] = 4;');
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportMatch(expect, actual, summary);

  exitFunc ('test');
}
