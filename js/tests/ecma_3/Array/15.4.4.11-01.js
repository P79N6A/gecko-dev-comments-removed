




































var bug = 312138;
var summary = 'Array.sort should not eat exceptions';
var actual = '';
var expect = '';

printBugNumber (bug);
printStatus (summary);

expect = "e=1 N=1";

var N = 0;
var array = [4,3,2,1];

try {
  array.sort(function() {
    throw ++N;
  }); 
} catch (e) {
  actual = ("e="+e+" N="+N);
}

reportCompare(expect, actual, summary);
