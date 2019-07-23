




































var gTestfile = 'regress-463334-01.js';

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

  u = 3;
  for (let i in (function() { for (var j=0;j<4;++j) { void u; yield; } })());
  
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
