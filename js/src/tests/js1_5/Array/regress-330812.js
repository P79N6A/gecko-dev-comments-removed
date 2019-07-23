




































var gTestfile = 'regress-330812.js';

var BUGNUMBER = 330812;
var summary = 'Making Array(1<<29).sort() less problematic';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

expectExitCode(0);
expectExitCode(3);

printStatus('This test passes if the browser does not hang or crash');
printStatus('This test expects exit code 0 or 3 to indicate out of memory');

try
{
  var result = Array(1 << 29).sort();
}
catch(ex)
{
  
  expect = 'InternalError: allocation size overflow';
  actual = ex + '';
}

reportCompare(expect, actual, summary);
