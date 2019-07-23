




































var bug = 315509;
var summary = 'Array.prototype.unshift on Arrays with holes';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var a = [0,1,2,3,4];
delete a[1];

expect = '0,,2,3,4';
actual = a.toString();

reportCompare(expect, actual, summary);

a.unshift('a','b');

expect = 'a,b,0,,2,3,4';
actual = a.toString();
  
reportCompare(expect, actual, summary);
