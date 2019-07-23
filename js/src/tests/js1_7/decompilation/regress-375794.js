




































var gTestfile = 'regress-375794.js';

var BUGNUMBER = 375794;
var summary = 'Decompilation of array comprehension with catch guard';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { try { } catch(a if [b for each (c in [])]) { } } ;
  expect = 'function() { try { } catch(a if [b for each (c in [])]) { } }';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
