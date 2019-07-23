




































var gTestfile = 'regress-465220.js';

var BUGNUMBER = 465220;
var summary = 'Do not assert: anti-nesting';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = 'TypeError: can\'t convert o to primitive type';

  jit(true);
 
  try
  {
    var o = {toString: function()(i > 2) ? this : "foo"};
    var s = "";
    for (var i = 0; i < 5; i++)
      s += o + o;
    print(s);
    actual = 'No Exception';
  }
  catch(ex)
  {
    actual = 'TypeError: can\'t convert o to primitive type';
  }
  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
