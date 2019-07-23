




































var gTestfile = 'regress-465460-01.js';

var BUGNUMBER = 465460;
var summary = 'TM: valueOf in a loop';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = '11111';

  jit(true);

  (function(d) { for (let j = 0; j < 5; ++j) { actual += ('' + d); } })({valueOf: function()1});

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
