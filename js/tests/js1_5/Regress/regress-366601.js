




































var gTestfile = 'regress-366601.js';

var BUGNUMBER = 366601;
var summary = 'Switch with more than 64k atoms';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var N = 100*1000;
  var src = 'var x = ["';
  var array = Array(N);
  for (var i = 0; i != N; ++i)
    array[i] = i;
  src += array.join('","')+'"];\n';
  src += 'switch (a) { case "a": case "b": case "c": return null; }  return x;';
  var f = Function('a', src);
  var r = f("a");
  if (r !== null)
    throw "Unexpected result: bad switch label";

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
