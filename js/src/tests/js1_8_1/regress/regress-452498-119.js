




































var gTestfile = 'regress-452498-119.js';

var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);






  function f() {
    var x;
    eval("for(let y in [false]) var x, x = 0");
  }
  f();




  new Function("for(x1 in ((function (){ yield x } )())){var c, x = []} function x(){} ");




  uneval(new Function("[(x = x) for (c in []) if ([{} for (x in [])])]"))



    function f() {
    var x;
    (function(){})();
    eval("if(x|=[]) {const x; }");
  }
  f();





  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
