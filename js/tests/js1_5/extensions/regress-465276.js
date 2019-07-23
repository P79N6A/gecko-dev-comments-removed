




































var gTestfile = 'regress-465276.js';

var BUGNUMBER = 465276;
var summary = '((1 * (1))|""';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  jit(true);

  expect = '[1, 1, 1, 1, 1, 1, 1, 1, 1, 1]';
  empty = [];
  out = [];
  for (var j=0;j<10;++j) { empty[42]; out.push((1 * (1)) | ""); }
  print(actual = uneval(out));

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
