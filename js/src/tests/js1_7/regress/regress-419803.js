





var BUGNUMBER = 419803;
var summary = 'Do not assert: sprop->parent == scope->lastProp';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function g() { for (var i=0; i<2; ++i) yield ({ p: 5, p: 7 }); }
  var iter = g();
  print(uneval(iter.next()));
  print(uneval(iter.next()));

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
