




































var gTestfile = 'regress-465460-11.js';

var BUGNUMBER = 465460;
var summary = 'TM: valueOf in a loop: do not assert';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for (let d=0;d<2;++d) for (let a=0;a<1;++a) "" + (d && function(){});

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
