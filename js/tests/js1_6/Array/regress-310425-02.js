




































var bug = 310425;
var summary = 'Array.indexOf/lastIndexOf edge cases';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);
  
expect = -1;
actual = Array(1).indexOf(1);
reportCompare(expect, actual, summary);
