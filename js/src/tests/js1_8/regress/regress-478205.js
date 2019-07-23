




































var gTestfile = 'regress-478205.js';

var BUGNUMBER = 478205;
var summary = 'Do not assert: p->isQuad()';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for each (let x in ['', '']) { switch([]) {} }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
