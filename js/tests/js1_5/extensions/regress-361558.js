




































var bug = 361558;
var summary = 'Assertion: sprop->setter != js_watch_set';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);
  
expect = actual = 'No Crash';

({}.__proto__.watch('x', print)); ({}.watch('x', print));

reportCompare(expect, actual, summary);
