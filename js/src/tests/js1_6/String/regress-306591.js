




































var gTestfile = 'regress-306591.js';

var BUGNUMBER = 306591;
var summary = 'String static methods';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
printStatus ('See https://bugzilla.mozilla.org/show_bug.cgi?id=304828');
 
expect = ['a', 'b', 'c'].toString();
actual = String.split(new String('abc'), '').toString();
reportCompare(expect, actual, summary +
              " String.split(new String('abc'), '')");

expect = '2';
actual = String.substring(new Number(123), 1, 2);
reportCompare(expect, actual, summary +
              " String.substring(new Number(123), 1, 2)");

expect = 'TRUE';
actual = String.toUpperCase(new Boolean(true)); 
reportCompare(expect, actual, summary +
              " String.toUpperCase(new Boolean(true))");


expect = (typeof window == 'undefined') ? 9 : -1;
actual = String.indexOf(null, 'l');             
reportCompare(expect, actual, summary +
              " String.indexOf(null, 'l')");

expect = 2;
actual = String.indexOf(String(null), 'l');             
reportCompare(expect, actual, summary +
              " String.indexOf(String(null), 'l')");

expect = ['a', 'b', 'c'].toString();
actual = String.split('abc', '').toString();
reportCompare(expect, actual, summary +
              " String.split('abc', '')");

expect = '2';
actual = String.substring(123, 1, 2);
reportCompare(expect, actual, summary +
              " String.substring(123, 1, 2)");

expect = 'TRUE';
actual = String.toUpperCase(true);
reportCompare(expect, actual, summary +
              " String.toUpperCase(true)");


expect = (typeof window == 'undefined') ? -1 : 11;
actual = String.indexOf(undefined, 'd');
reportCompare(expect, actual, summary +
              " String.indexOf(undefined, 'd')");

expect = 2;
actual = String.indexOf(String(undefined), 'd');
reportCompare(expect, actual, summary +
              " String.indexOf(String(undefined), 'd')");
