





































var gTestfile = 'regress-309840.js';

var BUGNUMBER = 309840;
var summary = 'Treat / in a literal regexp class as valid';
var actual = 'No error';
var expect = 'No error';

printBugNumber(BUGNUMBER);
printStatus (summary);

try
{ 
  var re = eval('/[/]/');
}
catch(e)
{
  actual = e.toString();
}

reportCompare(expect, actual, summary);
