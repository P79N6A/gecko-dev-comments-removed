




































var gTestfile = 'regress-461111.js';

var BUGNUMBER = 461111;
var summary = 'Decompilation of if (a,b)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f = (function() { if(a, b) { } });

  expect = 'function() { if(a, b) { } }';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
