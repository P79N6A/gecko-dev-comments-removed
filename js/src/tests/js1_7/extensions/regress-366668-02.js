




































var gTestfile = 'regress-366668-02.js';

var BUGNUMBER = 366668;
var summary = 'decompilation of "let with with" ';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;

  f = function() { let (w) { with({x: w.something }) { } } };
  expect = 'function() { let (w) { with({x: w.something }) { } } }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
