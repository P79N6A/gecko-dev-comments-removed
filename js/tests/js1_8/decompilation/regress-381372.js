




































var gTestfile = 'regress-381372.js';


var BUGNUMBER = 381372;
var summary = 'Decompilation of genexp in "with"';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = (function () { with (3 || (1 for (x in []))) {} });
  expect = 'function () { with (3 || (1 for (x in []))) {} }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
