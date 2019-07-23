




































var bug = 342359;
var summary = 'Overriding ReferenceError should stick';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

ReferenceError = 5;

try
{
  foo.blitz;
}
catch(ex)
{
  print(ex + '');
}

expect = 5;
actual = ReferenceError
  
reportCompare(expect, actual, summary);
