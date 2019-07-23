
























































var gTestfile = 'regress-118849.js';
var UBound = 0;
var BUGNUMBER = 118849;
var summary = 'Should not crash if we provide Function() with bad arguments'
  var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var cnFAIL_1 = 'LEGAL call to Function() caused an ERROR!!!';
var cnFAIL_2 = 'ILLEGAL call to Function() FAILED to cause an error';
var cnSTRING = 'ASDF';
var cnNUMBER = 123;





status = inSection(1);
actual = cnFAIL_1; 
try
{
  Function(cnSTRING);
  Function(cnNUMBER);  
  Function(cnSTRING,cnSTRING);
  Function(cnSTRING,cnNUMBER);
  Function(cnSTRING,cnSTRING,cnNUMBER);

  new Function(cnSTRING);
  new Function(cnNUMBER);
  new Function(cnSTRING,cnSTRING);
  new Function(cnSTRING,cnNUMBER);
  new Function(cnSTRING,cnSTRING,cnNUMBER);

  actual = expect;
}
catch(e)
{
}
addThis();








status = inSection(2);
actual = cnFAIL_2;
try
{
  Function(cnNUMBER,cnNUMBER); 
}
catch(e)
{
  actual = expect;
}
addThis();


status = inSection(3);
actual = cnFAIL_2;
try
{
  Function(cnNUMBER,cnSTRING,cnSTRING);
}
catch(e)
{
  actual = expect;
}
addThis();


status = inSection(4);
actual = cnFAIL_2;
try
{
  new Function(cnNUMBER,cnNUMBER);
}
catch(e)
{
  actual = expect;
}
addThis();


status = inSection(5);
actual = cnFAIL_2;
try
{
  new Function(cnNUMBER,cnSTRING,cnSTRING);
}
catch(e)
{
  actual = expect;
}
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
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (var i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
