




































var bug = 341496;
var summary = 'Iterators: check that adding properties does not crash';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var iter = Iterator({});
for (var i = 0; i != 10*1000; ++i)
        iter[i] = i;
  
reportCompare(expect, actual, summary);
