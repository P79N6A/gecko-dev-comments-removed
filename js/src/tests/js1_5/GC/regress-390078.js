




































var gTestfile = 'regress-390078.js';


var BUGNUMBER = 390078;
var summary = 'GC hazard with JSstackFrame.argv[-1]';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var a = new Array(10*1000);
  a[0] = { toString: function() { gc(); return ".*9"; }};;
  a[1] = "g";

  for (var i = 0; i != 10*1000; ++i) {
    String(new Number(123456789));
  }

  "".match.apply(123456789, a);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
