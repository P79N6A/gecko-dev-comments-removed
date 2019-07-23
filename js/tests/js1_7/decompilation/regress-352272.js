




































var gTestfile = 'regress-352272.js';

var BUGNUMBER = 352272;
var summary = 'decompilation of |let| in arg to lvalue returning function';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;
  f = function() { f(let (y = 3) 4)++; }
  expect = 'function() { f(let (y = 3) 4)++; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
