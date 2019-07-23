




































var gTestfile = 'regress-495907.js';

var BUGNUMBER = 495907;
var summary = 'Read upvar from trace-entry frame when created with top-level let';
var actual = ''
var expect = '00112233';



start_test();

var o = [];
for (let a = 0; a < 4; ++a) {
    (function () {for (var b = 0; b < 2; ++b) {o.push(a);}}());
}
actual = o.join("");

finish_test();


function start_test() {
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
  jit(true);
}

function finish_test() {
  jit(false);
  reportCompare(expect, actual, summary);
  exitFunc ('test');
}
