




































var gTestfile = 'regress-466262.js';

var BUGNUMBER = 466262;
var summary = 'Do not assert: f == f->root';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  var e = 1;
  for (var d = 0; d < 3; ++d) {
    if (d == 2) {
      e = "";
    }
  }
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
