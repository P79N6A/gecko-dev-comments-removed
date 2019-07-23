















































var gTestfile = 'regress-210682.js';
var UBound = 0;
var BUGNUMBER = 210682;
var summary = 'testing line ending with |continue| and only terminated by CR';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


for (i=0; i<100; i++)
{
  if (i%2 == 0) continue
    this.lasti = i;
}

status = inSection(1);
actual = lasti;
expect = 99;
addThis();




test();




function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc('test');
  printBugNumber(BUGNUMBER);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
