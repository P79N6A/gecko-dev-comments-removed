




































var gTestfile = 'regress-176125.js';

var BUGNUMBER = 176125;
var summary = 'if() should not return a value';
var actual = '';
var expect = 'undefined';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{
  printStatus('if (test1());');
  actual = typeof eval('if (test1());');
}
catch(ex)
{
  actual = ex + '';
}
 
reportCompare(expect, actual, summary + ': if (test1());');

try
{
  printStatus('if (test2());');
  actual = typeof eval('if (test2());');
}
catch(ex)
{
  actual = ex + '';
}
 
reportCompare(expect, actual, summary + ': if (test2());');


function test1()
{
  'Hi there!';
}

function test2()
{
  test1();
  return false;
}
