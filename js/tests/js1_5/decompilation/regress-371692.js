




































var gTestfile = 'regress-371692.js';

var BUGNUMBER = 371692;
var summary = 'Keep extra parentheses in conditional tests';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function (){ if((i=1)){ a=2; } };
  expect = 'function (){ if((i=1)){ a=2; } }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
