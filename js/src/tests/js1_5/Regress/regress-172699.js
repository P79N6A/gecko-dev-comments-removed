













































var gTestfile = 'regress-172699.js';
var UBound = 0;
var BUGNUMBER = 172699;
var summary = 'UTF-8 decoder should not accept overlong sequences';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];





var INVALID_CHAR = 0xFFFD;


status = inSection(1);
actual = decodeURI("%C0%AF").charCodeAt(0);
expect = INVALID_CHAR;
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
