




































var bug = 293782;
var summary = 'Local variables can cause predefined function object properties to be undefined';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

function f()
{
  var name=1;
}

expect = 'f';
actual = f.name;
  
reportCompare(expect, actual, summary);
