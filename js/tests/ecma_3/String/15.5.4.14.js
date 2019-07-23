




































var bug = 287630;
var summary = '15.5.4.14 - String.prototype.split(/()/)';
var actual = '';
var expect = ['a'].toString();

printBugNumber (bug);
printStatus (summary);

actual = 'a'.split(/()/).toString();
  
reportCompare(expect, actual, summary);
