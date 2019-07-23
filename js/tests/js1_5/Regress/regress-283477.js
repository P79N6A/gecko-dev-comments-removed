





































var gTestfile = 'regress-283477.js';

var BUGNUMBER = 283477;
var summary = 'a.lastIndexOf(b, c) should return -1 when there is no match';
var actual = '';
var expect = '';
var status;

enterFunc ('test');
printBugNumber(BUGNUMBER);
printStatus (summary);
 
status = summary + ' ' + inSection(1) + " " + '"".lastIndexOf("hello", 0);';
expect = -1;
actual = "".lastIndexOf("hello", 0);
reportCompare(expect, actual, status);

status = summary + ' ' + inSection(2) + " " + ' "".lastIndexOf("hello");';
expect = -1;
actual = "".lastIndexOf("hello");
reportCompare(expect, actual, status);

exitFunc ('test');
