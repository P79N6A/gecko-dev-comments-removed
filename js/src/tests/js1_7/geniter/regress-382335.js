




































var gTestfile = 'regress-382335.js';


var BUGNUMBER = 382335;
var summary = 'Trampolining threads using generators and iterators';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function make_gen() { yield 1; }

  var gen2 = make_gen();

  gen2.next();
  gen2.close();

  print(10);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
