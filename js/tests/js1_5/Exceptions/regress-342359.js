




































var gTestfile = 'regress-342359.js';

var BUGNUMBER = 342359;
var summary = 'Overriding ReferenceError should stick';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
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
