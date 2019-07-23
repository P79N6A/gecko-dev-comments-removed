





































var gTestfile = 'regress-280769-1.js';

var BUGNUMBER = 280769;
var summary = 'Do not crash on overflow of 64K boundary of [] offset in regexp search string ';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

status = summary + ' ' + inSection(1) + ' (new RegExp("zzz...[AB]").exec("zzz...A") ';

var N = 100 * 1000; 
var a = new Array(N + 1);
var prefix = a.join("z"); 
var str = prefix+"[AB]"; 
var re = new RegExp(str);
re.exec(prefix+"A");

reportCompare(expect, actual, status);
