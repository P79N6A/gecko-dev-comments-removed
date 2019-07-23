




































var bug = 314401;
var summary = 'setTimeout(eval,0,"",null)|setTimeout(Script,0,"",null) should not crash';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);
  
if (typeof setTimeout != 'undefined')
{
  try
  {
    setTimeout(eval, 0, '', null);
  }
  catch(ex)
  {
    printStatus(ex+'');
  }
}
reportCompare(expect, actual, 'setTimeout(eval, 0, "", null)');

if (typeof setTimeout != 'undefined' && typeof Script != 'undefined')
{
  try
  {
    setTimeout(Script, 0, '', null);
  }
  catch(ex)
  {
    printStatus(ex+'');
  }
}
reportCompare(expect, actual, 'setTimeout(Script, 0, "", null)');

if (typeof setInterval != 'undefined')
{
  try
  {
    setInterval(eval, 0, '', null);
  }
  catch(ex)
  {
    printStatus(ex+'');
  }
}
reportCompare(expect, actual, 'setInterval(eval, 0, "", null)');

if (typeof setInterval != 'undefined' && typeof Script != 'undefined')
{
  try
  {
    setInterval(Script, 0, '', null);
  }
  catch(ex)
  {
    printStatus(ex+'');
  }  
}
reportCompare(expect, actual, 'setInterval(Script, 0, "", null)');
