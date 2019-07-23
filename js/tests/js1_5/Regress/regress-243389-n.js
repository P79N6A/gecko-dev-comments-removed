





































var gTestfile = 'regress-243389-n.js';


var BUGNUMBER = 243389;
var summary = 'Don\'t crash on Regular Expression';
var actual = 'Crash';
var expect = 'error';

printBugNumber(BUGNUMBER);
printStatus (summary);




if (/(\\|/)/) {
}

actual = 'No Crash';

reportCompare(expect, actual, summary);
