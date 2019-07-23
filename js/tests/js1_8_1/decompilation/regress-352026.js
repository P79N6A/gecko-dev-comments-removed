





































var gTestfile = 'regress-352026.js';

var BUGNUMBER = 352026;
var summary = 'decompilation of yield in argument lists';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;
  f = function() { z((yield 3)) }
  expect = 'function() { z((yield 3)); }';
  actual = f + '';
  compareSource(expect, actual, summary + ': 1');

  f = function f(){g((let(a=b)c,d),e)}
  expect = 'function f(){g((let(a=b)c,d),e);}';
  actual = f + '';
  compareSource(expect, actual, summary + ': 2');

  f = function() { let(s=4){foo:"bar"} }  
  expect = 'function() { let(s=4){foo:"bar";} }';
  actual = f + '';
  compareSource(expect, actual, summary + ': 3');

  f = function() { (let(s=4){foo:"bar"}) }
  expect = 'function() { let(s=4)({foo:"bar"}); }';
  actual = f + '';
  compareSource(expect, actual, summary + ': 4');

  exitFunc ('test');
}
