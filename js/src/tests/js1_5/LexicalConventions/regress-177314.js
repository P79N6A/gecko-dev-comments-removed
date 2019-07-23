

















































var gTestfile = 'regress-177314.js';
var UBound = 0;
var BUGNUMBER = 177314;
var summary = "'\\" + "400' should lex as a 2-digit octal escape + '0'";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];



status = inSection(1);
actual = '\377';
expect = '\xFF';
addThis();


status = inSection(2);
actual = '\378';
expect = '\37' + '8';
addThis();


status = inSection(3);
actual = '\400';
expect = '\40' + '0';
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
