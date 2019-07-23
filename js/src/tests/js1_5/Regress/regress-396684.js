




































var gTestfile = 'regress-396684.js';

var BUGNUMBER = 396684;
var summary = 'Function call with stack arena exhausted';
var actual = '';
var expect = '';










test();


function f() {
  return arguments[arguments.length - 1];
}

function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 'PASS';

  var src = "return f(" +Array(10*1000).join("0,")+"Math.atan2());";

  var result = new Function(src)();

  if (typeof result != "number" || !isNaN(result))
    actual = "unexpected result: " + result;
  else
    actual = 'PASS';

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
