




































var bug = 312260;
var summary = 'Switch discriminant detecting case should not warn';
var actual = 'No warning';
var expect = 'No warning';

printBugNumber (bug);
printStatus (summary);

options('strict');
options('werror');

try
{
  switch ({}.foo) {}
}
catch(e)
{
  actual = e + '';
}

reportCompare(expect, actual, summary);
