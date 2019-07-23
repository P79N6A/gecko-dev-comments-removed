




































var gTestfile = 'regress-495773.js';

var BUGNUMBER = 495773;
var summary = 'Read upvar from trace-entry frame from JSStackFrame instead of tracing native stack';
var actual = ''
var expect = '010101';

function f() {
    var q = [];
    for (var a = 0; a < 3; ++a) {
        (function () {
            for (var b = 0; b < 2; ++b) {
                (function () {
                    for (var c = 0; c < 1; ++c) {
                        q.push(b);
                    }
                })();
            }
        })();
    }
    return q.join("");
}

test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);
  actual = f();
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
