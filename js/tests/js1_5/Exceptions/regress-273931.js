





































var gTestfile = 'regress-273931.js';

var BUGNUMBER = 273931;
var summary = 'Pop scope chain in exception handling';
var actual = '';
var expect = 'ReferenceError';

printBugNumber(BUGNUMBER);
printStatus (summary);

status = summary + ' ' + inSection(1) + ' ';
try
{
  with ({foo:"bar"})
    throw 42;
}
catch (e)
{
  try
  {
    printStatus(foo);
  }
  catch(ee)
  {
    actual = ee.name;
  }
}
 
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(2) + ' ';
try
{
  with ({foo:"bar"})
    eval("throw 42");
}
catch (e)
{
  try
  {
    printStatus(foo);
  }
  catch(ee)
  {
    actual = ee.name;
  }
}
 
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(3) + ' ';
try
{
  var s = "throw 42";
  with ({foo:"bar"})
    eval(s);
}
catch (e)
{
  try
  {
    printStatus(foo);
  }
  catch(ee)
  {
    actual = ee.name;
  }
}
 
reportCompare(expect, actual, status);
