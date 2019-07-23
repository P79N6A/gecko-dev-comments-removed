




































var gTestfile = 'regress-458959.js';

var BUGNUMBER = 458959;
var summary = 'this.JSON should not be enumerable';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  for (var i in this) 
  {
    if (i.toString() == 'JSON')
      actual = i;
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
