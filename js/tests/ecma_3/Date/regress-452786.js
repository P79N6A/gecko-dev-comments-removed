




































var gTestfile = 'regress-452786.js';

var BUGNUMBER = 452786;
var summary = 'Do not crash with (new Date()).getMonth.call(new Function())';
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
    (new Date()).getMonth.call(new Function());
  }
  catch(ex)
  {
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
