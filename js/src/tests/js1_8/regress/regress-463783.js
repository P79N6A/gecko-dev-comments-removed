





































var gTestfile = 'regress-463783.js';

var BUGNUMBER = 463783;
var summary = 'TM: Do not assert: "need a way to EOT now, since this is trace end": 0';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  var a = [0,1,2,3,4,5];
  for (let f in a);
  [(function(){})() for each (x in a) if (x)];

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
