




































var bug = 346904;
var summary = 'uneval expressions with double negation, negation decrement';
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
    '    return - --x;\n' +
    '}';
  try
  {
    f = eval('function () { return - --x; }');
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
    '    return - - x;\n' +
    '}';
  try
  {
    f = eval('function () { return - - x; }');
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    actual = ex + '';
    reportCompare(expect, actual, summary);
  }

  
  expect = 'SyntaxError';
  try
  {
    f = eval('function () { return ---x; }');
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    actual = ex.name;
    reportCompare(expect, actual, summary);
  }

  
  expect = 
    'function () {\n' +
    '    return + ++x;\n' +
    '}';
  try
  {
    f = eval('function () { return + ++x; }');
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
    '    return + + x;\n' +
    '}';
  try
  {
    f = eval('function () { return + + x; }');
    actual = f + '';
    compareSource(expect, actual, summary);
  }
  catch(ex)
  {
    actual = ex + '';
    reportCompare(expect, actual, summary);
  }

  
 
  expect = 'SyntaxError';
  try
  {
    f = eval('function () { return +++x; }');
    actual = f + '';
  }
  catch(ex)
  {
    actual = ex.name;
  }
  reportCompare(expect, actual, summary);


  exitFunc ('test');
}
