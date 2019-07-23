




































var bug = 309925;
var summary = 'Correctly parse Date strings with HH:MM';
var actual = new Date('Sep 24, 11:58 105') + '';
var expect = new Date('Sep 24, 11:58:00 105') + '';

printBugNumber (bug);
printStatus (summary);
  
reportCompare(expect, actual, summary);
