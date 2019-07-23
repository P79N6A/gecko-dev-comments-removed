




































var gTestfile = 'regress-352283.js';

var BUGNUMBER = 352283;
var summary = 'decompilation of |let| block with |while|, |let|';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f;
  f = function() { while(w) {let (x) { let y; } } }
  expect = 'function() { while(w) {let (x) { let y; } } }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
