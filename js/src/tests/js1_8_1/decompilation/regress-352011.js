





































var gTestfile = 'regress-352011.js';

var BUGNUMBER = 352011;
var summary = 'decompilation of statements that begin with object literals';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function() { ({}.y = i); }
  expect = 'function() { ({}.y = i); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function() { let(x) ({t:x}) }
  expect = 'function() { let(x) ({t:x}); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function() { (let(x) {y: z}) }
  expect = 'function() { let(x) ({y: z}); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
