





































var gTestfile = 'regress-346090.js';

var BUGNUMBER = 346090;
var summary = 'Do not crash with this regexp';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var r = /%((h[^l]+)|(l[^h]+)){0,2}?a/g;
  r.exec('%lld %d');

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
