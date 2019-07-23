




































var bug = 368224;
var summary = 'Assertion: pnprop->pn_type == TOK_COLON';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  ({ x: [] }) = #3={}

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
