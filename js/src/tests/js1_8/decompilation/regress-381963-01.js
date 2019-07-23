




































var gTestfile = 'regress-381963-01.js';

var BUGNUMBER = 381963;
var summary = 'Decompilation of genexp in |while|';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'function() { while(1, (2 for (x in []))) { break; } }';
  var f = function() { while(1, (2 for (x in []))) break; };
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
