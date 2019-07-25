






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



  (function(){for(var x in (x::window = x for (x in []))[[]]){}})();
  reportCompare(expect, actual, summary + ': 1');


  (eval("(function(){ this.watch(\"x\", function () { new function ()y } ); const y });"))();
  x = NaN;
  reportCompare(expect, actual, summary + ': 2');


  ({ set z(v){},  set y(v)--x, set w(v)--w });
  reportCompare(expect, actual, summary + ': 3');

  exitFunc ('test');
}
