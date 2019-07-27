





var BUGNUMBER = 352372;
var summary = 'Do not assert eval("setter/*...")';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  expect = "ReferenceError: setter is not defined, did you mean 'stop'?";
  try
  {
    eval("setter/*\n*/;");
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, 'eval("setter/*\n*/;")');

  try
  {
    eval("setter/*\n*/g");
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, 'eval("setter/*\n*/g")');

  try
  {
    eval("setter/*\n*/ ;");
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, 'eval("setter/*\n*/ ;")');

  try
  {
    eval("setter/*\n*/ g");
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, 'eval("setter/*\n*/ g")');

  exitFunc ('test');
}
