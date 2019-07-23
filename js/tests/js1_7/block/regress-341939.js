




































var bug = 341939;
var summary = 'Let block does not require semicolon';
var actual = '';
var expect = 'No Error';

printBugNumber (bug);
printStatus (summary);

try
{  
  eval('let (a) {} print(42);');
  actual = 'No Error';
}
catch(ex)
{
  actual = ex + '';
}

reportCompare(expect, actual, summary);
