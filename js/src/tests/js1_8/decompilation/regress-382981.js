




































var gTestfile = 'regress-382981.js';

var BUGNUMBER = 382981;
var summary = 'decompilation of expcio body with delete ++x';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'function () (++x, true)';
  var f = (function () delete ++x);
  actual = f + '';
  compareSource(expect, actual, summary);

  expect = 'function () (*, true)';
  var f = (function () delete *) ;
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
