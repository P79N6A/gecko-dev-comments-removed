




































var gTestfile = 'regress-452498-196.js';

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



  watch("x", Function);
  NaN = uneval({ get \u3056 (){ return undefined } });
  x+=NaN;

  reportCompare(expect, actual, summary + ': 1');



  (function (){
    var x;
    eval("const x; (function ()x)");
  }
    )();

  reportCompare(expect, actual, summary + ': 2');

  exitFunc ('test');
}
