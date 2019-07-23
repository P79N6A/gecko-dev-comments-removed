




































var gTestfile = 'regress-459990.js';

var BUGNUMBER = 459990;
var summary = 'Do not crash with if (true && a && b) { }';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  try
  {
    if (true && a && b) { }
  }
  catch(ex)
  {
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
