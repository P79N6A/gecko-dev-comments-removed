




































var gTestfile = 'regress-119719.js';

var BUGNUMBER = 119719;
var summary = 'Rethrown errors should have line number updated.';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

var err = new Error('this error was created on line 46');
try
{
  throw err; 
}
catch(e)
{
  expect = 49;
  actual = err.lineNumber;
} 
reportCompare(expect, actual, summary);
