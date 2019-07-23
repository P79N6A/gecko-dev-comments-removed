





































var bug = 280769;
var summary = 'Do not overflow 64K string offset';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

var N = 100 * 1000;

var prefix = new Array(N).join("a"); 

var suffix = "111";

var re = new RegExp(prefix+"0?"+suffix);  

var str_to_match = prefix+suffix;  

var index = str_to_match.search(re);

expect = 0;
actual = index;

reportCompare(expect, actual, summary);
