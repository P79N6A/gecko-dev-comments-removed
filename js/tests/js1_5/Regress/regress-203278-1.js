





































var gTestfile = 'regress-203278-1.js';

var BUGNUMBER = 203278;
var summary = 'Don\'t crash in recursive js_MarkGCThing';
var actual = 'FAIL';
var expect = 'PASS';

printBugNumber(BUGNUMBER);
printStatus (summary);

function test1() {}
function test() { test1.call(this); }
test.prototype = new test1();

var length = 1024 * 1024 - 1;
var obj = new test();
var first = obj;
for(var i = 0 ; i < length ; i++) {
  obj.next = new test();
  obj = obj.next;
}

actual = 'PASS';

reportCompare(expect, actual, summary);

