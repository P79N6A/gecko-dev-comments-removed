




































var bug = 353264;
var summary = 'Do not crash defining getter';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

this.x getter= function () { }; export x; x;
  
reportCompare(expect, actual, summary);
