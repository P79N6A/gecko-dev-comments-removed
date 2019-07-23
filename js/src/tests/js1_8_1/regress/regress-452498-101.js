




































var gTestfile = 'regress-452498-101.js';

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



  uneval(function(){with({functional: []}){x5, y = this;const y }});


  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
