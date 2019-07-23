




































var gTestfile = 'regress-461945.js';

var BUGNUMBER = 461945;
var summary = 'Do not assert: !ti->stackTypeMap.matches(ti_other->stackTypeMap)';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  [1 for each (x in {a:1, b:1, c:"", d:"", e:1, f:"", g:""}) if (0)];

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
