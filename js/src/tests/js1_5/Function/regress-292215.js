





































var gTestfile = 'regress-292215.js';

var BUGNUMBER = 292215;
var summary = 'Set arguments';
var actual = '';
var expect = '00012';

printBugNumber(BUGNUMBER);
printStatus (summary);
 

function zeroArguments () {
  arguments[1] = '0';
  actual += arguments[1];
}

function oneArgument (x) {
  arguments[1] = '1';
  actual += arguments[1];
}

function twoArguments (x,y) {
  arguments[1] = '2';
  actual += arguments[1];
}

zeroArguments();
zeroArguments(1);
zeroArguments('a', 'b');
oneArgument();
twoArguments();

reportCompare(expect, actual, summary);
