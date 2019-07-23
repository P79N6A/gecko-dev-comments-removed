




































var bug = 350268;
var summary = 'new Function with unbalanced braces';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  var f;

  try
  {
    expect = 'SyntaxError';
    actual = 'No Error';
    f = new Function("}");
  }
  catch(ex)
  {
    actual = ex.name;
  }
  reportCompare(expect, actual, summary);

  try
  {
    expect = 'SyntaxError';
    actual = 'No Error';
    f = new Function("}}}}}");
  }
  catch(ex)
  {
    actual = ex.name;
  }
  reportCompare(expect, actual, summary);

  try
  {
    expect = 'SyntaxError';
    actual = 'No Error';
    f = new Function("alert(6); } alert(5);");
  }
  catch(ex)
  {
    actual = ex.name;
  }
  reportCompare(expect, actual, summary);

  try
  {
    expect = 'SyntaxError';
    actual = 'No Error';
    f = new Function("} {");
  }
  catch(ex)
  {
    actual = ex.name;
  }
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
