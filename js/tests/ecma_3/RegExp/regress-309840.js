





































var bug = 309840;
var summary = 'Treat / in a literal regexp class as valid';
var actual = 'No error';
var expect = 'No error';

printBugNumber (bug);
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
