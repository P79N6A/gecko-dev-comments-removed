




































var gTestfile = 'regress-342359.js';

var BUGNUMBER = 342359;
var summary = 'Overriding ReferenceError should stick';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);


var SavedReferenceError = ReferenceError;

try
{
  ReferenceError = 5;
}
catch(ex)
{
}

try
{
  foo.blitz;
}
catch(ex)
{
  print(ex + '');
}

if (SavedReferenceError == ReferenceError)
{
  actual = expect = 'Test ignored due to bug 376957';
}
else
{
  expect = 5;
  actual = ReferenceError;
} 
reportCompare(expect, actual, summary);
