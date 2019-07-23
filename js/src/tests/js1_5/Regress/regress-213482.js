




































var gTestfile = 'regress-213482.js';

var BUGNUMBER = 213482;
var summary = 'Do not crash watching property when watcher sets property';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);
 
var testobj = {value: 'foo'};

function watched (a, b, c) {
  testobj.value = (new Date()).getTime();
}

function setTest() {
  testobj.value = 'b';
}

testobj.watch("value", watched);

setTest();

reportCompare(expect, actual, summary);
