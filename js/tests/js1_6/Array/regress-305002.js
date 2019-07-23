




































var bug = 305002;
var summary = '[].every(f) == true';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var notcalled = true;

function callback()
{
  notcalled = false;
}

expect = true;
actual = [].every(callback) && notcalled;

reportCompare(expect, actual, summary);
