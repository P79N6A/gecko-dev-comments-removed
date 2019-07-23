





































var gTestfile = 'regress-480147.js';

var BUGNUMBER = 480147;
var summary = 'TM: Do not assert: cx->bailExit';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  var w = [/a/, /b/, /c/, {}];
  for (var i = 0; i < w.length; ++i)
    "".replace(w[i], "");

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
