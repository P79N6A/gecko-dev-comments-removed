




































var gTestfile = 'regress-522123.js';

var BUGNUMBER = 522123;
var summary = 'Indirect eval confuses scope chain';
var actual = '';
var expect = '';



var x=1;

test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = 1;

  evil=eval;
  let (x=2) {
    actual = evil("x");
  };

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}

reportCompare(true, true);
