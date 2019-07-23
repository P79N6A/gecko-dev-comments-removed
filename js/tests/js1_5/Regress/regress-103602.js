























































var gTestfile = 'regress-103602.js';
var UBound = 0;
var BUGNUMBER = 103602;
var summary = 'Reassignment to a const is NOT an error per ECMA';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var cnFAIL_1 = 'Redeclaration of a const FAILED to cause an error';
var cnFAIL_2 = 'Reassigning to a const caused an ERROR! It should not!!!';
var sEval = '';






try
{
  sEval = 'const one = 1';
  eval(sEval);
}
catch(e)
{
  quit(); 
}


status = inSection(1);
try
{
  



  sEval = 'const one = 2;';
  eval(sEval);

  expect = ''; 
  actual = cnFAIL_1;
  addThis();
}
catch(e)
{
  
  actual = expect;
  addThis();
}


status = inSection(2);
try
{
  


  one = 2;
  actual = expect; 
  addThis();

  
  status = inSection(3);
  actual = one;
  expect = 1;
  addThis();

  
  status = inSection(4);
  actual = (one = 2);
  expect = 2;
  addThis();
}

catch(e)
{
  
  expect = '';
  actual = cnFAIL_2;
  addThis();
}




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
