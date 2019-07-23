




































var gTestfile = 'regress-344120.js';

var BUGNUMBER = 344120;
var summary = 'function to source with numeric labels';
var actual = '';
var expect;



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'function () { x = {1:1};}';
  actual = ''+function (){x={1:1}}

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
