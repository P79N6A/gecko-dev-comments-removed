





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
 
  ({ x: a }) = {}

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
