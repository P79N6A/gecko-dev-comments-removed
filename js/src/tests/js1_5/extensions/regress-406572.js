




































var gTestfile = 'regress-406572.js';

var BUGNUMBER = 406572;
var summary = 'JSOP_CLOSURE unconditionally replaces properties of the variable object - Browser only';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

if (typeof window != 'undefined')
{
  try
  {
    expect = 'TypeError: redeclaration of const document';
    var d = document;

    d.writeln(uneval(document));
    document = 1;
    d.writeln(uneval(document));

    if (1) 
      function document() { return 1; }

    d.writeln(uneval(document));
  }
  catch(ex)
  {
    actual = ex + '';
    print(actual);
  }
}
else
{
  expect = actual = 'Test can only run in a Gecko 1.9 browser or later.';
  print(actual);
}
reportCompare(expect, actual, summary);


