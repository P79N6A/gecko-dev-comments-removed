




































var bug = 322430;
var summary = 'Remove deprecated with statement warning';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

options('strict');
options('werrror');

expect = 'No Warning';

try
{
  var obj = {foo: 'baz'};
  
  
  
  
  
  eval('with (obj) { foo; }');

  actual = 'No Warning';
}
catch(ex)
{
  actual = ex + '';
}

reportCompare(expect, actual, summary);
