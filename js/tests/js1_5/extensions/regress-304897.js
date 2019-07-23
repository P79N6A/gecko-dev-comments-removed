




































var bug = 304897;
var summary = 'uneval("\\t"), uneval("\\x09")';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

expect = '"\\t"';
actual = uneval('\t');  
reportCompare(expect, actual, summary);

actual = uneval('\x09');  
reportCompare(expect, actual, summary);
