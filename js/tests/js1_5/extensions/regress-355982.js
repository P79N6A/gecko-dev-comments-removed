




































var bug = 355982;
var summary = 'Script("") should not fail';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
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
