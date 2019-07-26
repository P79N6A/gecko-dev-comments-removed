





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

  (function() {
    new function (){ for (var x = 0; x < 3; ++x){} };
  })();

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
