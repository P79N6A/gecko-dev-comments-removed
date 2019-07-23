




































var gTestfile = 'regress-352375.js';

var BUGNUMBER = 352375;
var summary = 'decompilation of for (4..x in [])';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { for (4..x in []) { } }  
  actual = f + '';
  expect = 'function() { for ((4).x in []) { } }   ';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
