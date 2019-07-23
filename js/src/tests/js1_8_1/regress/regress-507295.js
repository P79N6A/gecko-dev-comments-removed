




































var gTestfile = 'regress-507295.js';

var BUGNUMBER = 507295;
var summary = 'TM: assert with using result of assignment to closure var'
var actual = '';
var expect = 'do not crash';



start_test();
jit(true);

(function () {
    var y;
    (eval("(function () {\
               for (var x = 0; x < 3; ++x) {\
               ''.replace(/a/, (y = 3))\
               }\
           });\
     "))()
})()

jit(false);
actual = 'do not crash'
finish_test();


function start_test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
}

function finish_test()
{
  reportCompare(expect, actual, summary);
  exitFunc ('test');
}
