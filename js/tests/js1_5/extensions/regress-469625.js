





































var gTestfile = 'regress-469625.js';

var BUGNUMBER = 469625;
var summary = 'TM: Do not crash @ js_String_getelem';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  [].__proto__[0] = 'a';
  for (var j = 0; j < 3; ++j) [[, ]] = [];

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
