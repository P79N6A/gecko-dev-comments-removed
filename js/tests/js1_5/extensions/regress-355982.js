




































var gTestfile = 'regress-355982.js';

var BUGNUMBER = 355982;
var summary = 'Script("") should not fail';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'No Error';
  actual = 'No Error';
  try
  {
    if (typeof Script == 'undefined')
    {
      print('Test skipped. Script not defined.');
    }
    else
    {
      Script('');
    }
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
