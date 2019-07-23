




































var bug = 313153;
var summary = 'generic native method dispatcher extra actual arguments';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

expect = '1,2,3';
actual = (function (){return Array.concat.apply([], arguments)})(1,2,3).toString();
  
reportCompare(expect, actual, summary);
