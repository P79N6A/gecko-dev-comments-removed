







































var gTestfile = 'tostring-001.js';

test();

function test()
{
  var n0 = 1e23;
  var n1 = 5e22;
  var n2 = 1.6e24;

  printStatus ("Number formatting test.");
  printBugNumber ("11178");

  reportCompare ("1e+23", n0.toString(), "1e23 toString()");
  reportCompare ("5e+22", n1.toString(), "5e22 toString()");
  reportCompare ("1.6e+24", n2.toString(), "1.6e24 toString()");
   
}


