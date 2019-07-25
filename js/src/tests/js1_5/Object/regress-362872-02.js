







var BUGNUMBER = 362872;
var summary = 'script should not drop watchpoint that is in use';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
this.watch('x', function f() {
             print("before");
             x = 3;
             print("after");
           });
x = 3;

reportCompare(expect, actual, summary);
