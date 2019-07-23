




































var gTestfile = 'regress-507053.js';

var BUGNUMBER = 507053;
var summary = 'TM: invalid results with setting a closure variable in a loop'
var actual = '';
var expect = '2,4,8,16,32,2,4,8,16,32,2,4,8,16,32,2,4,8,16,32,2,4,8,16,32,';



start_test();
jit(true);

var f = function() {
  var p = 1;
  
  function g() {
    for (var i = 0; i < 5; ++i) {
      p = p * 2;
      actual += p + ',';
    }
  }
  g();
}

for (var i = 0; i < 5; ++i) {
  f();
}

jit(false);
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
