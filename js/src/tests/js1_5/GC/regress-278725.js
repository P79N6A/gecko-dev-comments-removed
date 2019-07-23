







































var gTestfile = 'regress-278725.js';


var BUGNUMBER = 278725;
var summary = 'Don\'t Crash during GC';
var actual = 'Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

var results = [];
for (var k = 0; k < 600000; k++) {
  if (! (k %100000)) {
    printStatus('hi');
    if (0) {
      results.length = 0;
      gc();
    }
  }
  results.push({});
}

actual = 'No Crash';
reportCompare(expect, actual, summary);
