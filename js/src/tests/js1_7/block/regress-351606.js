




































var gTestfile = 'regress-351606.js';

var BUGNUMBER = 351606;
var summary = 'Do not assert with nested for..in and throw';
var actual = 'No Crash';
var expect = 'No Crash';

enterFunc ('test');
printBugNumber(BUGNUMBER);
printStatus (summary);

(function () {for (let d in [1,2,3,4]) try { for (let a in [5,6,7,8]) (( function() { throw 9; } )()); } catch(c) {  }});

reportCompare(expect, actual, summary);


