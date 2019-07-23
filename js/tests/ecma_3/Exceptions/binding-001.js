





















































var gTestfile = 'binding-001.js';
var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Testing binding of function names';
var ERR_REF_YES = 'ReferenceError';
var ERR_REF_NO = 'did NOT generate a ReferenceError';
var statusitems = [];
var actualvalues = [];
var expectedvalues = [];
var status = summary;
var actual = ERR_REF_NO;
var expect= ERR_REF_YES;


try
{
  var f = function sum(){};
  print(sum);
}
catch (e)
{
  status = 'Section 1 of test';
  actual = e instanceof ReferenceError;
  expect = true;
  addThis();


  



  status = 'Section 2 of test';
  var match = e.toString().search(/ReferenceError/);
  actual = (match > -1);
  expect = true;
  addThis();
}




test();



function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = isReferenceError(actual);
  expectedvalues[UBound] = isReferenceError(expect);
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



function isReferenceError(bResult)
{
  return bResult? ERR_REF_YES : ERR_REF_NO;
}
