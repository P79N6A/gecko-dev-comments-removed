




































var bug = 311583;
var summary = 'Array.sort should skip holes and undefined during sort';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

var a = [, 1, , 2, undefined]; 

actual = a.sort().toString();
expect = '1,2,,,';  

reportCompare(expect, actual, summary);
