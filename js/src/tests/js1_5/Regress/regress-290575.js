




































var gTestfile = 'regress-290575.js';

var BUGNUMBER = 290575;
var summary = 'Do not crash calling function with more than 32768 arguments';
var actual = 'No Crash';
var expect = 'No Crash';
printBugNumber(BUGNUMBER);
printStatus (summary);

function crashMe(n) {
  var nasty, fn;

  nasty = [];
  while (n--)
    nasty.push("a"+n);   
  nasty.push("void 0");  
  fn = Function.apply(null, nasty);
  fn.toString();
}

printStatus('crashMe(0x8001)');

crashMe(0x8001);
 
reportCompare(expect, actual, summary);

function crashMe2(n) {
  var nasty = [], fn

    while (n--) nasty[n] = "a"+n
      fn = Function(nasty.join(), "void 0")
      fn.toString()
      }

printStatus('crashMe2(0x10000)');

summary = 'Syntax Error Function to string when more than 65536 arguments';
expect = 'Error';
try
{
  crashMe2(0x10000);
  actual = 'No Error';
  reportCompare(expect, actual, summary);
}
catch(e)
{
  actual = 'Error';
  reportCompare(expect, actual, summary);
  expect = 'SyntaxError';
  actual = e.name;
  reportCompare(expect, actual, summary);
}

