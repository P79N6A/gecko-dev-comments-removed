





var BUGNUMBER = 457065;
var summary = 'Do not assert: !fp->callee || fp->thisp == fp->argv[-1].toObjectOrNull()';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  var e = eval;
  for (var a in this) { }
  (function() { eval("this; for (let b in [0,1,2]) { }"); })();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
