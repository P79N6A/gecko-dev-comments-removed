




































var bug = 293782;
var summary = 'Local variables should not be enumerable properties of the function';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

function f()
{
  var x,y
}

var p;
actual = '';

for (p in f)
{
  actual += p + ',';
}
expect = 'prototype,';
  
reportCompare(expect, actual, summary);
