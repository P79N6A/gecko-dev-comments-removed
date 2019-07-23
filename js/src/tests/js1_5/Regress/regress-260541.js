





































var gTestfile = 'regress-260541.js';


var BUGNUMBER = 260541;
var summary = 'Recursive Error object should not crash';
var actual = 'Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
var myErr = new Error( "Error Text" );
myErr.name = myErr;

actual = 'No Crash';

reportCompare(expect, actual, summary);
