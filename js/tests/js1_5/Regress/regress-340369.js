





































var bug = 340369;
var summary = 'Oh for crying out loud.';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

try
{  
  eval('return /;');
}
catch(ex)
{
  print(ex+'');
}

reportCompare(expect, actual, summary);
