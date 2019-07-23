




































var bug = 319384;
var summary = 'Do not crash converting string to number';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

printStatus('This test only runs in the browser');

if (typeof clearTimeout === 'function')
{
  try
  {
    clearTimeout('foo');
  }
  catch(ex)
  {
  }
}
  
reportCompare(expect, actual, summary);
