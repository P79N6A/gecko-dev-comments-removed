




































var gTestfile = 'regress-451946.js';

var BUGNUMBER = 451946;
var summary = 'Do not crash with SELinux execheap protection';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  print('This test is only valid with SELinux targetted policy with exeheap protection');

  jit(true);

  var i; for (i = 0; i  < 2000000; i++) {;}

  jit(false);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
