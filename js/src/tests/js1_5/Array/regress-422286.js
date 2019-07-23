




































var gTestfile = 'regress-422286.js';

var BUGNUMBER = 422286;
var summary = 'Array slice when array\'s length is assigned';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  Array(10000).slice(1);
  a = Array(1); 
  a.length = 10000; 
  a.slice(1);
  a = Array(1); 
  a.length = 10000; 
  a.slice(-1);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
