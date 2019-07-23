




































var bug = 343713;
var summary = 'Do not JS_Assert with nested function evaluation';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);
  
with (this) 
with (this) {
  eval("function outer() { function inner() { " +
       "print('inner');} inner(); print('outer');} outer()");
}

reportCompare(expect, actual, summary);
