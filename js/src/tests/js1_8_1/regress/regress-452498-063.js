




































var gTestfile = 'regress-452498-063.js';

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



  function f(that) {
    for (ix in this)
      print(ix);
    for (let ix in that)
      ;
  }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
