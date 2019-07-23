




































var gTestfile = 'regress-375639.js';

var BUGNUMBER = 375639;
var summary = 'Uneval|Decompilation of string containing null character';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  print('normalize \\x00 to \\0 to hide 1.8.1 vs 1.9.0 ' +
        'branch differences.');

  expect = '"a\\' + '0b"';
  actual =  uneval('a\0b').replace(/\\x00/g, '\\0');
  reportCompare(expect, actual, summary + ': 1');

  expect = 'function () { return "a\\0b"; }';
  actual = ((function () { return "a\0b"; }) + '').replace(/\\x00/g, '\\0');;
  compareSource(expect, actual, summary + ': 2');

  exitFunc ('test');
}
