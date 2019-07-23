




































var gTestfile = 'regress-483103.js';

var BUGNUMBER = 483103;
var summary = 'TM: Do not assert: p->isQuad()';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  var t = new String("");
  for (var j = 0; j < 3; ++j) {
    var e = t["-1"];
  }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
