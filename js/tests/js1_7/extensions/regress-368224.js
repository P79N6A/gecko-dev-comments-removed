




































var gTestfile = 'regress-368224.js';

var BUGNUMBER = 368224;
var summary = 'Do not assert: pnprop->pn_type == TOK_COLON';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  ({ x: [] }) = #3={}

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
