




































var bug = 312354;
var summary = '11.13.1 Simple Assignment should return type of RHS';
var actual = '';
var expect = '';




printBugNumber (bug);
printStatus (summary);

var re = /x/g;
var y = re.lastIndex = "7"
  
expect = "string";
actual = typeof y;

reportCompare(expect, actual, summary);
