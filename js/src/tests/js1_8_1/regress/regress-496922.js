




































var gTestfile = 'regress-496922.js';

var BUGNUMBER = 496922;
var summary = 'Incorrect handling of extra arguments';
var actual = ''
var expect = '0,0,1,1,2,2,3,3';





enterFunc ('test');
printBugNumber(BUGNUMBER);
printStatus (summary);
jit(true);

var a = [];
let(
f = function() {
    for (let x = 0; x < 4; ++x) {
        (function() {
            for (let y = 0; y < 2; ++y) {
              a.push(x);
            }
        })()
    }
}) { (function() {})()
    f(99)
}
actual = '' + a;

jit(false);
reportCompare(expect, actual, summary);
exitFunc ('test');


