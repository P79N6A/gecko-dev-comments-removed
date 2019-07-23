




































var gTestfile = 'regress-381205.js';


var BUGNUMBER = 381205;
var summary = 'uneval with special getter functions';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
expect = '({get x p() {print(4);}})';
getter function p() { print(4) }
actual =  uneval({x getter: this.__lookupGetter__("p")});
reportCompare(expect, actual, summary + ': global');
