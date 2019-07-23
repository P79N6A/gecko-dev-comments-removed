














































var gTestfile = 'regress-204210.js';
var UBound = 0;
var BUGNUMBER = 204210;
var summary = "eval() is not a constructor, but don't crash on |new eval();|";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];

printBugNumber(BUGNUMBER);
printStatus(summary);









try
{
  var x = new eval();
  new eval();
}
catch(e)
{
}





f();
function f()
{
  try
  {
    var x = new eval();
    new eval();
  }
  catch(e)
  {
  }
}





var s = '';
s += 'try';
s += '{';
s += '  var x = new eval();';
s += '  new eval();';
s += '}';
s += 'catch(e)';
s += '{';
s += '}';
eval(s);





s = '';
s += 'function g()';
s += '{';
s += '  try';
s += '  {';
s += '    var x = new eval();';
s += '    new eval();';
s += '  }';
s += '  catch(e)';
s += '  {';
s += '  }';
s += '}';
s += 'g();';
eval(s);


function h()
{
  var s = '';
  s += 'function f()';
  s += '{';
  s += '  try';
  s += '  {';
  s += '    var x = new eval();';
  s += '    new eval();';
  s += '  }';
  s += '  catch(e)';
  s += '  {';
  s += '  }';
  s += '}';
  s += 'f();';
  eval(s);
}
h();

reportCompare('No Crash', 'No Crash', '');
