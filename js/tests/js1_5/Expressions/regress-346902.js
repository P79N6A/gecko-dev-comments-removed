




































var bug = 346902;
var summary = 'uneval expressions with object literals';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);
  
  var f;

  expect = 
    'function () {\n' +
    '    ({}[alert(5)]);\n' +
    '}';
  try
  {
    f = eval('function () { 1 ? {}[alert(5)] : 0; }');
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    actual = ex + '';
    reportCompare(expect, actual, summary);
  }

  expect = 
    'function () {\n' +
    '    [alert(5)];\n' +
    '}';
  try
  {
    f = eval('function () { {}[alert(5)]; }');
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    actual = ex + '';
    reportCompare(expect, actual, summary);
  }

  exitFunc ('test');
}
