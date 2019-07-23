




































var gTestfile = 'regress-352008.js';

var BUGNUMBER = 352008;
var summary = 'decompilation of |for([k,v] in o| should not assert';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  printStatus('Repeat after me: destructuring decompilation isn\'t working yet. ;-)');
  printStatus('bug 352008 is fixed if this test does not assert');

  var f;
  f = function() { for each([k, v] in o) { } }
  expect = 'function() { for each([k, v] in o) { } }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
