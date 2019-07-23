




































var gTestfile = 'regress-331719.js';

var BUGNUMBER = 331719;
var summary = 'Problem with String.replace running with WAY_TOO_MUCH_GC';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
print('This test requires WAY_TOO_MUCH_GC');
 
expect = 'No';
actual = 'No'.replace(/\&\&/g, '&');

reportCompare(expect, actual, summary);
