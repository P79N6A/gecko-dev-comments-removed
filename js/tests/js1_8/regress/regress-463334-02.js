




































var gTestfile = 'regress-463334-02.js';

var BUGNUMBER = 463334;
var summary = 'TM: Do not crash in isPromoteInt';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  for (let i in 
         (function() { for (let j = 0; j < 4; ++j) with({t: NaN}) yield; })()) 
  { 
  }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
