




































var gTestfile = 'regress-462388.js';

var BUGNUMBER = 462388;
var summary = 'Do not assert: JSVAL_TAG(v) == JSVAL_STRING';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  var c = 0, v; for each (let x in ["",v,v,v]) { for (c=0;c<4;++c) { } }

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
