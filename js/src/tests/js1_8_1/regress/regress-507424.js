




































var gTestfile = 'regress-507424.js';

var BUGNUMBER = 507424;
var summary = 'TM: assert with regexp literal inside closure'
var actual = '';
var expect = 'do not crash';



start_test();
jit(true);

(new Function("'a'.replace(/a/,function(x){return(/x/ for each(y in[x]))})"))();

jit(false);
actual = 'do not crash'
finish_test();


function start_test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
}

function finish_test()
{
  reportCompare(expect, actual, summary);
  exitFunc ('test');
}
