




































var gTestfile = 'regress-348810.js';

var BUGNUMBER = 348810;
var summary = 'Do not crash when sorting an array of holes';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var a = Array(1);
  a.sort();
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
