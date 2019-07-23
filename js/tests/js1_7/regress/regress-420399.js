




































var gTestfile = 'regress-420399.js';

var BUGNUMBER = 420399;
var summary = 'Let expression error involving undefined';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = /TypeError: \(let \(a = undefined\) a\) (is undefined|has no properties)/;
  try
  {
    (let (a=undefined) a).b = 3;
  }
  catch(ex)
  {
    actual = ex + '';
  }

  reportMatch(expect, actual, summary);

  exitFunc ('test');
}
