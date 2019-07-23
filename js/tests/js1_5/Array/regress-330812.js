




































var bug = 330812;
var summary = 'Making Array(1<<29).sort() less problematic';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

expectExitCode(0);
expectExitCode(3);

printStatus('This test passes if the browser does not hang or crash');
printStatus('This test expects exit code 0 or 3 to indicate out of memory');

var result = Array(1 << 29).sort();

reportCompare(expect, actual, summary);
