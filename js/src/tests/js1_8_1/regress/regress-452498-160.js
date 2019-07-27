





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


  (eval("(function(){ this.watch(\"x\", function () { new function ()y } ); const y });"))();
  x = NaN;
  reportCompare(expect, actual, summary + ': 2');


  ({ set z(v){},  set y(v)--x, set w(v)--w });
  reportCompare(expect, actual, summary + ': 3');

  exitFunc ('test');
}
