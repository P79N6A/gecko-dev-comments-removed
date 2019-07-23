





































var gTestfile = 'regress-254974.js';





var BUGNUMBER = 254974;
var summary = 'all var and arg properties should be JSPROP_SHARED';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

function testfunc(tokens) {
  function eek(y) {} 
  return tokens.split(/\]?(?:\[|$)/).shift();
}
bad=testfunc;
function testfunc(tokens) {
  return tokens.split(/\]?(?:\[|$)/).shift();
}
good=testfunc;

var goodvalue = good("DIV[@id=\"test\"]");
var badvalue = bad("DIV[@id=\"test\"]");

printStatus(goodvalue);
printStatus(badvalue);

expect = goodvalue;
actual = badvalue;
 
reportCompare(expect, actual, summary);
