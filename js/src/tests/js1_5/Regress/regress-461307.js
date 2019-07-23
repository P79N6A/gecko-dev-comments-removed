




































var gTestfile = 'regress-461307.js';

var BUGNUMBER = 461307;
var summary = 'Do not crash @ QuoteString';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  jit(true);

  print(function() { for(/x/[''] in []) { } });


  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
