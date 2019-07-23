






var gTestfile = 'template.js';

var BUGNUMBER = 99999;
var summary = '';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
