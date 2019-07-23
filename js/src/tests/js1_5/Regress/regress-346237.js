




































var gTestfile = 'regress-346237.js';

var BUGNUMBER = 346237;
var summary = 'RegExp - /(|)??x/g.exec("y")';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  /(|)??x/g.exec("y");

  reportCompare(expect, actual, summary + ': /(|)??x/g.exec("y")');

  exitFunc ('test');
}
